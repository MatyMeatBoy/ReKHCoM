# Renderer reset recovery inventory - 2026-10-27

Owner: codex-hud-invalidation. Queue: optiport-hud-invalidation.
Scope implemented: explicit HUD invalidation only. No reset handler is added.
No evidence presently connects renderer reset to the reported return-to-window
crash. Focus/background events alone must not be treated as device loss.

## Implemented boundary

`PortHdHudInvalidate()` is a renderer-thread API for an explicit renderer
change or asset re-import, outside a draw. It frees HUD GPU resources, queued
content-keyed draws and renderer ownership via Shutdown, then re-arms the load
attempt and language state. The next Prepare loads against the current renderer.
`PortHdHudShutdown()` now also clears sExportCount with its freed hash array.
Otherwise an export session retained the old count across a newly allocated
empty array, including remaining permanently full after reaching 2048 entries.

Shutdown alone deliberately preserves sAttempted: Prepare calls it on texture
upload failure, and resetting that flag there would retry a failed decode/upload
every frame. Only the explicit invalidation boundary authorizes another attempt.
The API is intentionally not wired to a partial global reset handler yet.

Verification:
- `python port/tools/test_hd_hud_invalidation.py`: PASS. Extracted production
  Shutdown/Prepare/Invalidate, injected texture generations and upload failure.
  Covers content-keyed assets and draw queues, export count/arrays, repeated
  teardown, recovery into a new texture generation, and thirty frames without
  retry after failure. The baseline failed the export-count assertion first.
- Full modified HUD C source syntax check for Linux with real SDL3 headers,
  llvm-mingw clang Linux target/sysroot and PLATFORM_PC/VERSION_EU: PASS.
- `git diff --check`: PASS. No heavy build or installed main changes.

## Native 32-bit recovery still required

PumpEvents in port_main.c handles SDL_EVENT_RENDER_DEVICE_RESET only under
EMSCRIPTEN, calling PortWebHdClear. A complete native reset must inventory and
clear all renderer-owned resources, then let their existing creation paths run:

| Owner | Resources / re-arm requirements |
| --- | --- |
| port_main.c | sTexture, main/screen/HUD overlay textures, overlay, border, margin menu/fade textures, blur |
| port_main.c local statics | Tex in a helper near line1404; Present HD2D, menu alpha, sample and notice textures. Move ownership into reachable reset state or expose owner reset hooks; resetting only globals is incomplete. |
| port_hdsprites.c | sprite pairs/cache, title art/bar, ring, menu lights; Shutdown clears textures, readiness and queued draws |
| port_hdhud.c | use explicit Invalidate, not bare Shutdown, to re-arm attempts |
| port_hdlayout.c | imported textures plus hosted supersample gauge targets; another active budget lane owns these changes |
| HD text | GDI, FreeType and guest implementations have separate Shutdown paths; glyph GPU caches must be included while preserving valid font initialization |
| Other UI owners | achievement toast and scene-gallery preview textures; review movie-local textures when reset is consumed inside playback loops |

Device reset and targets reset have different SDL semantics. The complete path
must invalidate readiness before native pixels are omitted again, handle pending
queues, and verify recreated uploads before reporting HD resources usable. A test
that merely pushes an SDL reset event does not establish these ownership rules.

## Guest64 / host bridge recovery still required

Linux64 and Android64 host Pump currently ignore renderer reset events. The guest
SDL shim retains CPU RGBA pixels per Tex and lazy nearest/smooth host IDs, so it
can re-upload without rereading optional assets. Host texture IDs currently are
plain reusable slot indexes (0..4095); host update/free access the slot directly.
Blindly clearing host slots permits an old guest ID to mutate or free a new
texture that reused that slot.

A complete contract should explicitly communicate a renderer generation, invalidate
the guest nearest/smooth IDs for the old generation, and reject stale-generation
update/free/draw at the host. Validate these transitions with extracted production
producer/consumer functions, including a slot deliberately reused after reset.
Maintain CPU RGBA ownership and do not free new-generation IDs from old wrappers.

Bridge entries are append-only and every host must implement a new entry before
the guest calls it. An alternative is a documented app_state bitfield with bit0
background and upper bits generation; RefreshTouch must mask background rather
than treating any nonzero generation as background. Do not silently reinterpret
existing app_state semantics or alias reset state with background/quit. No bridge
contract change is part of this commit; this inventory is a follow-up design.
