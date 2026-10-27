# Host HD music progress and Linux64 movie audit — 2026-10-27

Task `host-music-progress`, owner `codex-host-music`, branch
`fix/host-music-progress`; base `43987205a`.

## Host music implemented

`HdNext` limits both the ordinary decode and the first decode after looping to
`loopEnd - pos`. Previously a short loop read 1024 samples after every seek,
including samples beyond the configured endpoint. Failed seek or zero PCM after
one rewind/retry is terminal for this opened track. The producer marks `failed`
and stops playback; subsequent requests return silence without decoder retries.
It does not close/free Vorbis or file memory. PLAY returns 0 while failed. Open
requires a valid rate, a nonempty track and two initial PCM samples.

Guest fallback depends on PR #288 (`fix/optiport-music`, `f74323e4a`): its PLAY
failure path closes the track and preserves cartridge music gain. This branch
intentionally leaves `port/src/port_hdmusic.c` unchanged.

Ownership: in Linux64, Android64 and PS5 hosts `H_audio_push` synchronously calls
`HdMusicPush`; it is a game bridge call, not an SDL audio callback. Open, PLAY,
Close and that producer execute serially on the calling game thread. SDL receives
copied PCM and never owns Vorbis. Producer failure only marks state; Close/Open
free it when the producer is no longer active. A future host which mixes on a
separate thread must supply synchronization before adopting this interface.

Validation: `python port/tools/test_host_music_failures.py` extracts and executes
the actual shared functions with decoder/file fault injection. It failed before
the change and passes afterward. Covers 1000 expected samples of loop [100,200),
96000 requests after decoder failure (at most two decodes and one seek), failed
seek, consumer Push, no producer-side close, terminal PLAY=0, Open prefill failure
cleanup and a successful Open/PLAY control. The complete header also compiles
against its real bundled stb_vorbis. No large builds or device playback claimed.

## Linux64 ffmpeg audit: still pending, separate work

Observed on the same base in `port/linux64/hdmovie_ffmpeg.inc`:

- Lines 185–200 retain the audio clock after PCM EOF, even when no audio samples
  were ever produced. The clock stays fixed; delays grow with each video frame.
  For 300 frames at 30fps with zero PCM, the production delay formula sums to
  1495 seconds for nominal 10 seconds. Audio exhaustion must switch to a
  continuous active wall clock, including the case of an absent audio track.
- ffprobe `fgets`, both media `fread` calls and child `pclose` calls are blocking
  on the event thread. A child which stops producing without closing its pipe
  prevents skip/close events from being processed. Explicit process ownership,
  interruptible partial reads, cancellation and bounded child collection need a
  separate contract; adding a startup timeout alone does not repair this.
- `played=1` precedes unchecked `SDL_UpdateTexture`, another readiness gap in
  this Linux64 backend, independent of Android's readiness fix.

Suggested next evidence: controlled fake ffprobe/ffmpeg processes producing
partial frames, PCM EOF, no audio track, silent stalls and a child ignoring
termination. Measure event cancellation latency, maximum queued PCM, bytes of
partial frame state, live child PIDs and file descriptors after teardown. Keep
buffers bounded, retain backpressure and distinguish EOF, transient no data and
technical failure. Do not infer Deck behavior from a Windows harness; no Linux
pipe/device experiment was performed in this audit.
