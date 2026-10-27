# Native renderer reset recovery - 2026-10-27

Owner: codex-native-renderer-reset. Queue task: optiport-native-renderer-reset.
Branch: fix/native-renderer-reset. Base after HUD API merge: 3af38a313.

This is resource-lifecycle hardening. There is no confirmed causal connection
between SDL device loss and the user's reported return-to-window crash.

## Implemented ownership boundary

Native hosted PumpEvents dispatches render reset events on SDL's main thread.
Device reset first detaches the active render target. If detaching fails, it
retains pending state and suppresses Present until a later detach succeeds;
resources are never destroyed while that target is still attached. GPU calls
from workers are not authorized by this API.

After a successful detach, the reset retires all 17 main-owned SDL textures and
zeros their creation dimensions. Previously local skip-layer, HD2D, menu-alpha,
font-sample and notice textures now have reachable owner state. The test checks
the complete set of static main texture declarations against its inventory.
CPU rendering buffers and simulation state are retained.

The reset also invokes the existing sprite/title/ring/light and layout/gauge
shutdown paths, explicit HUD invalidation, and HD text shutdown (the selected
native GDI/FreeType implementation). New narrow owner hooks retire the toast
texture while preserving its queue, and the gallery texture while preserving
CPU preview data and pending decoder work. Fresh texture generation is logged.
No changes to port_hdlayout.c are included; target cleanup comes from PR296.

TARGETS_RESET preserves ordinary textures/readiness. The only native SDL render
targets are the layout supersampling targets; their existing producer clears and
redraws them every pass. A loss of target contents therefore needs a normal redraw,
not a full asset re-import. Web reset handling remains its existing path; guest64
continues to need its separate explicit generation/host-ID contract.

## Movies and paused screens

Native Windows and Linux movie input loops now consume reset events. Device loss
aborts HD playback, cleans its local video/subtitle/pause textures, and returns
failure to the cartridge fallback even if HD playback had already started. A
pause window closes through a resume/no-skip UI path. Targets-only reset keeps
video playback intact. Audio hold cleanup keeps its original played flag, so
failure return does not bypass subtitle cleanup or audio restoration.

Native Android32 registers the portable decoder's renderer-reset callback during
renderer initialization. Dependency: HdMovieAndroidSetRendererResetCallback from
the parallel Android media-reset lane must accompany this commit when linking
Android32. The portable decoder invokes the callback on its renderer thread,
cleans local textures and returns failure; this branch does not edit that file.

Options and link-idle may present an already-rendered frame while simulation is
paused. They remain interactive and may temporarily lose an HD decoration whose
native pixels were omitted before device loss. Recovery does not run game logic
to regenerate OAM. On resume, normal game/OAM setup and resource readiness choose
usable replacements or native fallback. This is an accepted visual limit, not a
claim of pixel-perfect recovery of frozen frames.

## Verification and limits

- New production main reset harness failed against the absent baseline handler,
  then passes. Covers 17 old-generation main textures, all six module boundaries,
  targets-only behavior, main-thread restriction, failed target detach followed
  by recovery, repeated loss and idempotent servicing.
- Production owner harness passes: toast queue retained, CPU gallery preview
  retained, actual movie event consumer distinguishes device/targets reset and
  normal pause input; both native PlayFile result expressions return failure
  after device loss; UI close hook never requests a story skip.
- Production HUD invalidation harness passes: shutdown/export cleanup, explicit
  generation recreation and thirty failed-upload frames with no repeated retry.
- Existing sprite first-row native fallback, layout readiness, movie failure
  injection and Windows PCM seek harnesses pass. Layout harness uses the explicit
  llvm-mingw clang path in CC because clang is not on this shell's PATH.
- All five modified C translation units pass syntax checks with real SDL3 headers
  for Windows i686 (including -mno-ms-bitfields) and Linux i686 prepared sysroot.
- git diff --check passes. No heavy builds, package changes or installed main
  changes were performed.

The tests execute extracted production ownership and consumer functions with
fault injection; they do not force a real GPU/driver reset. They do not prove
visual recovery on reporter hardware or exercise a full application frame loop.
The coordinator's next isolated build/test cycle validates integration.
