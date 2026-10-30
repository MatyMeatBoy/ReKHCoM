# Scene framing implementation plan

> Execute inline with superpowers:executing-plans. No implementation workers.

**Goal:** Default Ajuste camera for engine scenes, with independent UI and the
current wide behavior available as experimental Extendido.
**Architecture:** A pure scene-policy/scale header selects integer camera scale
from the actual viewport. Presenter derives world and HUD scales separately;
configuration stores one new scalar outside PortConfig's guest ABI. A reviewed
manifest records the exact current build versions admitted as honors exceptions.
**Tech:** Existing C/SDL3 presenter and PPU, C++ launcher, Python QA.
**Spec:** ../specs/2026-10-30-scene-adjust-framing-design.md (approved30Oct).

## Constraints and review focus

- Preserve gameplay, timing, art, original/CRT view, field/battle and movies.
- Ajuste default0, Extendido1; no edits to saved zoom or HUD size for camera.
- Do not infer honors from historical prose, latest proposal or ordinary approval.
- UI can have a smaller pixel scale than the new camera; PPU must support it.
- Resize, narrow GBA scenes, tall/ultrawide, scene exit and menu opening must not
  leave the camera or HUD override active. Respect minimum window240x160.
- No desktop takeover after Escape. Native game captures only. Build jobs1 and
  BelowNormal. ROM/SAV/Atlas/installations and releases remain protected.

## Tasks

1. **Policy and setting.** New port/include/port/scene_framing.h and reviewed
   port/progress/SCENE_FRAMING_HONORS.json; gPortSceneFraming in port_config.c,
   declared in port.h without struct growth. Add scene_framing INI key0..1.
   Tests compile actual config consumer and header: default, old INI, persistence,
   bounds, strict honors, native complete and integer scales across resolutions.
2. **Presenter and UI separation.** UpdateResolution in port_main.c derives
   scene camera and user HUD independently; active only for gModeEvent and Auto,
   outside options canvas. Integer scale also used for GBA-size scene fit.
   gPortSceneCameraAdjusted enables existing docked-HUD path in narrow scenes.
   Compiled test extracts actual UpdateResolution with SDL/PPU seams, verifying
   scales, native bounds, transitions, honors, battle, menu and Original controls.
3. **Options and wording.** Add native scene-framing row, two values and short
   explanation in six languages; launcher row uses same key and default.
   Compile actual launcher consumer for save/reopen. Keep i18n generator changes
   limited to these new strings. Preserve the provider/online settings branch.
4. **Actual game validation.** Build WindowsEU32, use native scene capture in
   fresh QA directories to compare a non-honors scene and an honors scene, with
   dialogs and zoom4/6 at720/1080. Add1440/2160 geometry tests and resize/exit
   controls. Distinguish compiled64 consumer checks from an actual64 game build.
5. **Delivery.** Fresh checks, inspect exact diff, PR and push for2.3; integrate
   only with queue lock after adequate verification. Update board with evidence
   and any unverified platforms. No new release or overwrite of existing ZIPs.

## Ledger

- Policy/config tests pass with32/64-bit compilers: independent scene_framing0/1,
  legacy default, persistence/clamp, integer native-complete viewport and manifest
  of10current honors build versions verified read-only from Atlas.
- Presenter, launcher row and actual message-consumer compiled tests pass.
  First actual game run was RED: debug event selector enters gModeEventDebug,
  not gModeEvent (both named Event). Added exact mode support with RED/GREEN test;
  run02 confirms320x180 scene007 at1080 and720, baseline Extended pixel parity
  and honor121 parity. Failed run01 retained.
- run03 atframe900 revealed a second RED integration case: native dialog still
  used the world scale despite independent HUD settings. Reused the existing
  screen layer for the full message composition (BG/portrait/text/choices), at
  user scale and its matching dock margin. Logical camera policy independent
  of options canvas drives tagging; cleanup clears BG ownership. Actual message
  and presenter consumer regressions now GREEN. Build91/91 passes; run04 native
  dialog comparisons in progress. Do not claim UI split before its captures pass.
- Native diagnostics/scripts/captures live in pruebas/scene-adjust-v23, fresh
  isolated dirs, dummy SDL/software, one process/BelowNormal. No desktop input.
  No64-bit game/device certification yet. No release or installed game edits.

- User approved design and clarified equivalent GBA integer framing rather than
  a physical x6 on every screen. Separate issue330 documentation work is PR531.
- Read-only current Atlas queue+SQLite review differs from historical claims:
  107/108 current build is ordinary approved, while109 and121/122 are honors.
  Manifest will use actual current build_version records, not historical labels.
- Initial current honors event candidates:004,008,053,054,055,074,102,109,121,122.
  Shared-map variants are not automatically admitted as additional events.
