# Movie master volume (#449)

The game mixer and HD music already apply `masterVolume`. Movie players open a
separate stream, resume it at SDL's default gain of 1, and (where implemented)
set a fade-only gain. This change snapshots the current master percentage at
movie entry, sets it before playback, and multiplies the existing fade by it.
PCM production, queue accounting, clock, media files, codecs and resolution are
unchanged. A rejected initial gain returns the existing replacement-failure
result before presenting the movie.

Five decoder routes are covered: Windows32 Media Foundation, its extracted
Windows64 host decoder, Linux32 FFmpeg, Linux64 host FFmpeg, and shared Android
MediaCodec (32/64 callers). Linux64 has no existing movie fade; this change
preserves its pause/skip behavior and applies master gain only.

## Mixed host/guest compatibility

`bridge_list.h` and the four-string `hd_movie` signature stay unchanged. Optional
`HF_thread` commands 0x210/0x211 advertise exact magic `MOV1` and set a bounded
integer percentage. The existing thread operations use 0..4, GPU commands 0x100,
and game-audio commands 0x200..0x204. Unknown thread operations return zero or an
unsupported sentinel, neither accepted as the capability magic.

| Guest / host | Behavior |
| --- | --- |
| New / new | Guest sends current 0..100 master; host consumes it once at movie entry. |
| Old / new | No pending value: legacy default 100, preserving the old ABI. |
| New / old | At master 100, legacy movie call remains usable. Otherwise decline HD replacement and preserve the original fallback. |
| Old / old | Unchanged; this pair does not acquire the fix. |

The pending value is consumed before path validation/decoder execution, so
failure, skip or reset cannot leak a muted setting into a later legacy call.
Invalid percentages discard a pending value. Movie commands do not alter the
game stream or negotiate the separate game-audio lifecycle protocol.

No new staging header is introduced: producers already track/copy
`audio_bridge.h`. Android's added argument is internal to the codec and its two
callers, compiled together; the serialized bridge signature is unchanged.
SDL 3.4.18 documents `SDL_SetAudioStreamGain` as safe from any thread. The new
bridge command handles integers only and invokes no SDL API.

## Validation and limits

- `test_movie_master_volume.py`: RED on the original producer's default-volume
  resume; GREEN for all movie open/fade sites and 166 compiled CPU/SDL-dummy
  checks: master 0/50/100, fade, pause/resume, queued PCM, mixed capabilities,
  one-shot consumption, invalid values and decoder failure/quit outcomes.
- `test_bridge_movie_reset.py`: actual Android wrapper/native caller reset,
  skip and return mapping. Its older Linux event fixture was updated for the
  already merged configurable Start/mouse/touch events.
- Existing game-audio bridge probe passes SDL hosts and ring controls;
  production movie controls/clock and 17 fragmented-read cases pass.
- Eight changed production TUs compile with cached Windows/Linux/Android target
  toolchains; Windows64's complete MF host links successfully.
- Windows functional lease: 12 real EU32/64 game cases plus three actual MF
  reset/missing/corrupt controls pass. Both 07d5 baselines deliver nonzero PCM
  with master 0; both candidates deliver entirely silent, nonempty output while
  decoding all 60 frames and returning to Event. Master 50/100 steady movie RMS
  ratio is 0.5 within 0.001; pause/cancel preserves all 60 frames and confirmed
  skip fades the PCM before returning to Event (45/39 frames). Reset preserves
  renderer generation/clip recovery and the one-shot value resets to legacy100.

`WINDOWS-VERIFIED.json` records 59 postconditions and raw paths. The first private
observer required every normal EOF envelope to fall below 80%; the original32
control also lacks a complete EOF fade on this fixture. Verified entry and
confirmed-skip fades are reported separately. The 64-bit master100 recording
includes a short pre-movie transient, so gain comparison uses median 50ms RMS
windows in the steady movie interval 0.4..1.6s, not whole-file peak. Raw samples
remain intact. Software late-frame counts are recorded without a timing claim.

Private commands, RED log and target receipts live in
`pruebas/issue449-movie-master-volume/`. These are CPU, source and compilation
results, plus actual Windows MF playback to the SDL software/disk sink. Physical
listening, Deck hardware and Android device/API16 validation remain unverified.
`test_movie_master_volume.py --work DIR` lets an independent reviewer put all
probe outputs in their own directory. No published artifact is replaced.
The omitted basic-import `_123` scene and launcher copy
in #449 are separate follow-up work, not resolved by this audio change.
