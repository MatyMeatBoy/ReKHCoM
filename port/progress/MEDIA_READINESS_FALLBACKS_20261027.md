# Media fallback readiness — 2026-10-27

Branch: `fix/media-readiness-fallbacks`. Owner: `codex-media-fallbacks`.

Android used to set `played=1` as soon as the codec opened, before a texture or
frame upload succeeded. Texture creation and upload errors could consequently
consume the HD scene as a black replacement instead of allowing cartridge video.
It now accepts the replacement after a successful first upload, returns failure
on texture/upload error, always frees subtitle resources, and preserves app close
as return value 2 even before the first frame. A fixed five second wall clock
limits a decoder which starts but never supplies a usable first frame. The limit
is an initial conservative bound, not a measured decoder startup guarantee.

Windows checks the three playback MF attribute/media-type creation calls before
using their COM pointers and leaves their cleanup at the existing done label.

Verification (silent, passed):
- `python port/tools/test_hd_movie_failures.py`: extracted production Android
  block, all four upload formats, allocation and upload failure, successful
  readiness, image/texture cleanup; extracted MF guards, failure and success;
  extracted first-frame watchdog, just before/at deadline and after readiness.
  The Android allocation-failure test failed against the previous implementation.
- `python port/tools/test_hd_movie_seek.py`: existing Windows PCM seek regression.
- Full modified Android source syntax check with NDK r30 clang,
  `--target=aarch64-linux-android24`, real SDL3 and FreeType headers.
- Full modified Windows source syntax check with requested llvm-mingw clang,
  `-target i686-w64-mingw32`, real SDL3 and FreeType headers and `-iquote include`.
- `git diff --check`.

No Android device playback or Wine/Deck playback is claimed by these tests.
No main installation files, releases, or binaries were changed.
