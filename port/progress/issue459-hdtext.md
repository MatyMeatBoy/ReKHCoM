# Linux HD text: speculative warmup must not disable the backend

The #459 reporter confirmed the compatible `--safe-startup` path works, but
normal startup still fails and HD text remains pixelated. These remain separate
conditions. Fresh failed normal-launch logs were requested; this text correction
does not identify the normal-startup cause or certify Gaming Mode hardware.

## Mechanism and correction

The portable FreeType backend disables HD text when a required glyph cannot be
drawn, allowing the PPU to resume the original lettering. Its startup warmup also
enumerated every byte32–255 and optional Japanese-encoded arrows. Undefined
CP1252 entries or an absent optional CJK face therefore disabled the entire HD
backend before any game lettering requested those characters.

Warmup now checks the selected face and actual fallback coverage before asking
for a glyph. The same check covers both UI scales, gameplay scales, numeric
cells and optional arrows. It does not alter font selection, spacing, GBA text
semantics, or the existing fallback when an actually requested glyph fails.
The latter now logs the missing Unicode codepoint once when HD becomes disabled.

The correction applies wherever this portable FreeType backend is compiled.
The GDI/browser/stb backends retain their existing warmup policy through the
default predicate. Inspection found their missing-glyph paths do not have this
same FreeType global-readiness invalidation; upload failures are separate.

## Verification and limits

- `port/tools/test_hdtext_warmup.c` includes the production rasterizer and shared
  warmup, with real SDL software textures, FreeType and shipped outline fonts.
  The width table and optional CJK absence are controlled dependencies. Before
  the fix, gameplay and UI warmup both invalidate readiness. Afterward both
  preserve readiness and a usable ASCII texture. A genuinely requested missing
  glyph still activates the original-text fallback. This is a component probe,
  not execution of the game or a physical display.
- Fresh native Linux32 EU/US/JP cores built successfully. Eight isolated game
  routes compared old/new US with HD on/off, new EU HD, JP on/off without CJK,
  and the unchanged desktop64 core. All completed. The VM used512MiB/oneCPU,
  snapshot storage, software drawing and dummy audio; it cannot certify audible
  continuity, SteamOS libraries or physical Gaming Mode.
- The meaningful rendering control is `Mode_Deck`: the old HD setting produced
  the same pixels as original mode. The corrected US frame emitted15 flowing
  and57 small glyphs and displayed HD labels/counters. Original-mode captures
  remained pixel-identical before/after. A private Arial fallback fixture was
  provided for this comparison; it is not redistributed and is not a claim to
  match the reporter's system font. JP correctly retained original lettering
  without a usable CJK face.
- EXP notices intentionally retain original pixel-art digits in current source;
  their unchanged captures are controls, not evidence of failed HD replacement.
  An initial desktop64 capture used unsupported guest readback; the corrected
  observer used the host presentation capture. Neither observation failure was
  classified as a game crash.
- Shared core-asset packaging, launcher resource/permission packaging, three
  unified-package tests,29 launcher regressions and ten startup-stage controls
  pass. The first launcher invocation lacked a POSIX shell/tools on Windows;
  supplying Git's shell and tool path fixed the test environment without a
  production wrapper change.

For a native Linux development environment with SDL3/FreeType headers installed:

```sh
clang -O2 -ffunction-sections -fdata-sections -Iport/include \
  $(pkg-config --cflags sdl3 freetype2) port/tools/test_hdtext_warmup.c \
  -Wl,--gc-sections $(pkg-config --libs sdl3 freetype2) -o warmup-probe
SDL_VIDEO_DRIVER=dummy SDL_AUDIO_DRIVER=dummy ./warmup-probe /path/to/game
```

The delivered test must retain `--safe-startup` for the reporter's current
working route. Keep #459 open for that test and the normal-launch evidence.

## Delivery — 29/10/2026 UTC

[v2.2.5-test459-hdtext.1](https://github.com/MatyMeatBoy/ReKHCoM/releases/tag/v2.2.5-test459-hdtext.1)
is public and was sent to #459 ([6486570914](https://github.com/MatyMeatBoy/ReKHCoM/issues/459#issuecomment-6486570914))
and #330 ([6486571271](https://github.com/MatyMeatBoy/ReKHCoM/issues/330#issuecomment-6486571271)).
The35332743-byte TAR has SHA256
`f7f5f8c6339a9ee9c98384aefb1893cc477c66f25c0b98beefa6a4e5d1fc9eb0`.
All five public assets were downloaded anonymously and matched the closed local
files. Tag50ed2618bf84c0e723c80ed911f2fdba561713de includes #489; compiled
native32 source is c87838bcb13e5eaca1f267fdce0a3abee156d457. Desktop64 reuse is
identified separately in SOURCE.json. Stable2.2.5 remains Latest and unchanged.

The exact final TAR passed four additional isolated game routes: safe native32
EU/US/JP and desktop64 US, deck captures, Quit81 and preserved safe-startup
preferences. EU/US emitted15 flowing and57 small HD glyphs; JP retained original
lettering without CJK. Reporter HD confirmation and failed normal-launch logs
remain pending; neither issue was closed on the strength of VM checks.
