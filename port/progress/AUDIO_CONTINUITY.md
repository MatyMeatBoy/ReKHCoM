# Audio priming and bounded underrun recovery

The 64-bit SDL bridge started playback immediately while its SDL shim treated
pause/resume and gain as successful no-ops. The game tried to prime four frames
but the host was already consuming them. Its queued-count adapter also treated
SDL input-frame counts as device-frame counts and converted them a second time.
At 63072/65536 Hz, 8192 actual input bytes were reported as 10760/11184 bytes.
PS4's already-resampled ring really does report device frames; removing the
conversion globally would break its existing contract.

An optional AUD1 capability uses existing `HF_thread` commands, without changing
the fixed bridge table. Windows/Linux/Android64 SDL hosts advertise input-frame
units and implement pause/resume/gain. The guest probes before opening, starts
paused, primes its original four frames, and resumes explicitly. Old hosts and
ring hosts retain the previous contract. New hosts still start automatically
for old guests. Windows/Linux movie adapters no longer bypass priming on movie
return when the new guest owns the audio lifecycle. A matched rebuilt host and
guest are required for these fixes; an old pair is not retroactively changed.

The shared SDL consumer handles missing PCM with a five-millisecond ramp of its
last queued sample toward silence, then a short fade-in when fresh PCM returns.
If production resumes partway through a ramp, the ramp finishes before recovery.
It never repeats an old block or advances the game/mixer clock. Normal producer
PCM is forwarded byte-for-byte. The consumer uses fixed 16 KiB scratch storage
per stream, bounds each demand to 2048 frames and performs no game calls,
decoder work or I/O. Stream locks serialize producer, consumer and reset state.
SDL's own stream queue operations retain SDL's internal allocation behavior.
Recovery handles stereo F32/S16; other valid SDL formats are forwarded natively
without recovery, rather than rejected by the optional helper's format limits.

This consumer is used by hosted game audio and hosted HD music, and by the
three SDL bridge hosts after host-side HD mixing. Hosted HD music fills its
existing roughly 200 ms queue before first resume. Its flow log includes the
consumer underrun count: synthesized silence cannot turn a starvation into a
false clean result. Reset clears the old tail; movie/focus pause remains an
intentional pause. Console ring implementations and the web keep their existing
output implementations; their clocks and resamplers are not modified here.

Verification so far:

- `test_guest_audio_bridge.py`: production guest and three SDL host functions,
  including the actual optional-command dispatch, pass against real SDL.
  Opening paused, gain, input units at three rates, priming, and Windows movie
  return are checked. The actual PS4 `GamePush` with a controlled ring sink
  preserves device-frame conversion as a legacy control.
- The same contract test passes with the physical Windows WASAPI backend,
  using only zero PCM during playback: no audible test tone or system-loopback
  recording. All three host-code bodies are exercised on Windows, not on Linux
  or Android devices. Ninety steady producer frames report zero underruns.
- `test_audio_continuity.py`: real SDL stream PCM consumption fails abruptly
  without the fallback and passes with it. F32 complete/partial starvation,
  S16, byte-exact steady PCM, reset, failed producer/callback/recovery writes,
  frequency changes during a ramp and INT_MAX demand are covered. For the
  controlled 0.6-to-silence F32 signal, the maximum adjacent step is 0.0025001;
  the S16 case stays within 69/32768. These are synthetic signal measurements,
  not an assertion about every game's naturally changing waveform.
  A rejected consumer enqueue discards its old tail so a later callback cannot
  resurrect a nonzero sample after the device has fallen back to silence.
- Existing hosted HD decoder/output-failure and loader tests pass. The updated
  flow analyzer distinguishes queued PCM from recorded smooth-recovery events.
- Complete Linux64 and Android64 host translation units and x32/AArch64 guest
  audio/music/shim units pass syntax checks. Windows64 host builds and its
  ABI/boot contracts pass.
- Frozen private Windows EU builds at `f0e33ea6c` pass ten D3D11 game runs:
  two published-pre2 controls and eight fixed 32/64-bit cases spanning GBA/HD
  title music, normal playback and a controlled 250 ms stall of only the test
  game's main thread. All reach the title and produce nonzero PCM. Both GBA
  producer PCM and sample-rate/frame-count records remain byte-identical to
  the corresponding controls, including the stalled cases. Normal cases record
  no fallback interventions; deliberate stalls record one to three recoveries.
  Hosted HD music actually plays track6 and the host opens the imported Title
  track; a filename appearing in an asset log alone is not counted as playback.
  These disk-driver recordings are silent local tests, not physical listening
  confirmation. An earlier ten-run lifecycle probe was rejected as audio
  evidence because it stopped before `TitleFinishIntro` and produced zero PCM;
  the corrected scenario presses A again after entering the title mode and
  requires a nonzero-signal gate.

These results establish concrete lifecycle and fallback defects. They do not
prove that the original user's audible hitch has only one cause, or certify
audio quality on every physical device. During a producer stall longer than
the available queued PCM, future game audio does not exist yet: recovery fades
to silence rather than inventing or indefinitely repeating it. Device loss or
a failed SDL enqueue can still prevent output. Keep reporter confirmation and
platform gameplay checks separate from the local PCM regressions.
