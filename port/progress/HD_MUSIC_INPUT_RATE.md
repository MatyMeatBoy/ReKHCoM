# HD music input-rate contract

The 64-bit guest opens audio at 48000 Hz, then queues GBA PCM at 63072 or
65536 Hz. Its stream adapter already adjusts playback frequency, but the host
HD mixer kept the original open rate. Mixing a 48000-Hz track into one second
of 63072 input frames therefore consumed 63072 track frames: 31.4% too fast
(36.53% at 65536). This matches the class of report in issue410; the reporter's
exact architecture and physical playback confirmation remain separate.

The guest now reports its current input PCM rate through an optional command
in the existing thread bridge. All six shared HD music hosts use that rate for
HD resampling. This is independent of lifecycle capability and queued-count
units: console ring hosts continue reporting device frames, and SDL hosts
continue reporting input frames. Queue PLL correction and the game's PCM,
clock, playback ratio, pause and gain remain unchanged. Older hosts ignore the
unknown command; older guests retain the previous host-open rate behaviour.
Matched rebuilt host and guest are required for the correction.

Validation:

- A production HdMusicMix/HdMusicPush probe counted63072/65536 decoded track
  frames for one second of the corresponding input PCM before correction.
- The real guest format-change function, shared host command dispatcher and
  production mixer pass24 combinations of four input rates, two track rates
  and three PLL ratios. Each second consumes the track's sample rate within
  one interpolation frame, while original GBA PCM bytes are unchanged.
- Invalid rates cannot replace a valid rate. Core queries, unknown commands,
  disabled HD and older-guest controls retain their contracts.
- All six hosts include the HD mixer before the common thread dispatcher.
  Existing SDL lifecycle/unit, legacy ring and nine HD decoder/movie-failure
  regressions pass. This is source/CPU coverage, not console-device testing.

Final Windows build, exact-package playback checks and reporter confirmation
are still required before claiming delivery or physical audio verification.
