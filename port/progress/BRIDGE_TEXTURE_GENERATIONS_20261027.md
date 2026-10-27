# Bridge SDL GPU generations — 2026-10-27

Task `bridge-texture-generations`, owner `codex-bridge-reset`.
Branch `fix/bridge-texture-generations`, base `8a6c9016c`.

## Contract and implementation

Linux64/Android64 keep opaque integer texture handles in 4096 slots. Handles
now encode a 12-bit slot and 19-bit allocation serial. Update, free and draw
validate the entire handle. Reusing a slot cannot let an old wrapper mutate,
free or draw a replacement texture. An exhausted serial retires the slot;
serials never wrap. Initial uploads are checked before a handle is committed.

`SDL_EVENT_RENDER_DEVICE_RESET` destroys host-owned bridge GPU textures and
advances the renderer epoch. Android's transient host toast is also destroyed
and cleared; the next notification recreates it. Device reset during host movie
playback advances the same epoch and aborts HD playback with public result 0,
so the cartridge scene remains available. Movie-local video/subtitle textures
are cleaned by their existing owner, not inserted into the bridge slot table.
Explicit skip/close preserve their existing semantics. If Android loses the
renderer while waiting in the background, its callback runs immediately but the
movie remains blocked until foreground (or quit), then tears down and returns 0.
This avoids starting the fallback while the app is still backgrounded.

The existing `getenv` bridge entry has one reserved optional capability query:
`KHCOM_RENDERER_GENERATION` -> `bridge-textures-v1:<uint32 decimal epoch>`.
The guest queries it once per input drain after the host pumps events. Each Tex
wrapper checks its cached epoch locally: old nearest/smooth IDs are discarded
without sending frees, while CPU RGBA ownership stays intact. The next draw
recreates the requested GPU filter from those pixels. No host query is added to
each draw/update/free. On epoch exhaustion the host retires all slots rather
than ever issuing aliased IDs.

## Compatibility / boundaries

No bridge entry, signature, table size or header layout changed. `app_state`
remains exactly 0/1 background; `set_title` is unchanged. An older host returns
unset (or an invalid value), which the new guest treats as unsupported. Other
bridge platforms retain existing texture behavior and do not need to implement
this query. Older guests can still use the new host's opaque handles, but do not
recover GPU contents after reset because they lack the epoch query. The host
still refuses their stale operations, protecting unrelated new textures.

TARGETS_RESET does not invalidate this host path: bridge textures are STATIC,
movie video textures are STREAMING and subtitles are STATIC; none is a render
target. Rendering affinity remains the existing game/renderer bridge thread.
The guest's existing lazy GPU allocation policy is retained; a failed re-upload
returns failed draw, not a claim that a device has recovered.

Native Android32 must register its global renderer reset hook through
`HdMovieAndroidSetRendererResetCallback(void (*callback)(void))`, implemented by
this branch. The separate native reset worker owns that registration in
`port_main.c`. This branch does not modify port_main.c and does not independently
claim complete native32 renderer recovery.

## Evidence

- `test_bridge_texture_generations.py --baseline` reproduces old stale-slot
  aliasing and exits 1 against the exact base. Updated production host/guest
  functions pass: stale update/free/draw after slot reuse, both filters, CPU
  re-upload, old/malformed query behavior, upload failure cleanup, idempotent
  wrapper destruction, no per-draw query, retired serial and epoch exhaustion.
- `test_bridge_movie_reset.py` passes extracted Android dispatch/callback,
  nested background/foreground handling, targets-reset no-op, and actual
  Android32/Android64 PlayFile/host wrapper return classification. Linux event
  dispatch and reset-over-skip return priority are exercised too.
- Existing `test_hd_movie_failures.py` passes with reset result 3 mapped to
  public 0, including failure after an earlier successful upload.
- Existing `test_linux64_movie_streaming.py` clock regression passes.
- Full source syntax checks pass: Linux64 host with Linux sysroot and SDL3,
  Android64 host and shared movie player with real NDK/SDL3/FreeType, and the
  freestanding x32 guest shim with its actual guest headers. Linux has its
  pre-existing stb_vorbis pointer-comparison warning.
- `git diff --check` passes. No heavy builds, installed main changes, device
  reset reproduction or hardware recovery confirmation was performed here.
