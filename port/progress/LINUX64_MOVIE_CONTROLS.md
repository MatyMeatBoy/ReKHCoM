# Linux64 movie controls and video FPS

The Linux64 movie host previously consumed A, B or START as an immediate skip.
Its private video loop also bypassed the guest's enabled FPS notice. Issue421
reported both differences from Windows.

START, Escape, either configured START key or a tap now opens the host-owned
pause window. Continue returns to the picture; Skip scene opens Yes/No with No
selected initially. A/B outside the window cannot skip the scene. The paused
picture is captured before presenting, avoiding undefined backbuffer contents.
The decoded audio device waits during the window and the wall-clock epoch is
shifted by the same elapsed pause, without subtracting or replacing PCM samples.
The fallback clock after audio EOF and videos without an audio device use the
same shift. Decoder pipes remain bounded by their existing OS backpressure.

The window uses the existing subtitle font rasterizer, with SDL debug text as a
font-unavailable fallback. Language follows the guest's subtitle identity when
available, then the forwarded `KHCOM_LANGUAGE` command-line language, then the
remembered 1..5 game language. Automatic configuration plus `--language es` is
covered even with subtitles disabled. A normal first-time in-game language
choice is remembered by `src/title/mode_lang.c` before the movie can play.

With `show_fps=1`, the host draws a distinct `Video … FPS` notice for actual
presented movie frames. It does not label video presentation as guest/game FPS;
pause time is excluded from its sampling epoch.

`python port/tools/test_linux64_movie_controls.py` exercises the production
event consumer, confirmation decisions, language resolution, existing movie
clock and exact pause shift. It also compiles the entire new UI against SDL3
headers when the existing SDK is present, without launching SDL or linking a
full host. The observed RED was the absent pause implementation; the updated
CPU checks pass. The CPU/header checks are separate from the runtime evidence
below.

Existing harness controls
`KHCOM_HDMOVIE_PAUSETEST=<seconds>` and
`KHCOM_HDMOVIE_PAUSE_ACTION=cancel` can walk the pause and confirmation windows;
they write `hdmovie_pause.bmp` and `hdmovie_skip_confirm.bmp` in the isolated
data directory.

## Executed runtime release checks

The candidate package was compiled from
`ace4116e97e54eff34ca4cb2035ea46d2be1f261`. Its exact Linux64 host SHA256 is
`b5fb7bb8119336b53c4f15f8dd07d0e0c9815604769a1e0c4732f2d4f4992908`.
This later evidence update changes documentation only; the compiled product
sources are unchanged.

Twelve cases ran in the existing Debian12 snapshot with 512 MiB/one CPU, SDL
software/dummy rendering and disk PCM. An owned miniature RKG5 guest calls the
exact packaged host's real movie bridge. Real Debian FFmpeg/ffprobe 5.1.9 decodes
a three-second 640x360 MPEG4/AAC fixture with a constant 440 Hz tone; no ROM, game
save or proprietary movie is used.

| Cases | Observed result |
| --- | --- |
| Baseline | 90 frames, nonzero decoded PCM |
| Pause/cancel/continue, FPSoff and FPSon | 90 frames in both cases |
| Explicit Yes | 19 frames, confirmed skip |
| Missing font; silent movie | 90 frames after continue in each case |
| A/B outside pause |Four proven SDL event injections, 90 frames, no skip |
| Quit; renderer reset during playback |Injected after 10 presents; 10 frames, expected consume/fallback results |
| Resize during pause |Actual window and both captures960x540, 90 frames after continue |
| Quit; renderer reset during pause |Injected after the first UI present; pause quit/device-reset logs, 19 frames, expected consume/fallback results |

The FPSon/off pause captures differ in 1584 pixels, all inside the notice's
rectangle `(6,6)-(228,22)`, with zero differences outside it. The normal tone's
longest internal zero gap is one sample; the cancel cases contain 0.832/0.853 s
of silence during the window and resume afterward. The separate real POSIX
1080p producer/read-consumer test completed 154 reads with 27 empty reads and
exactly 27 waits, preserving the event/audio pump after every fragment.

Quit/reset/resize faults use an explicit test-only SDL interposer, not a real
GPU device loss. The checks establish local decoder, UI and pause-clock
behavior. They do not certify physical Steam Deck/GamingMode GPU performance,
story progression or perceptual audio quality. Reporter confirmation remains
separate.

An initial VM lacked FFmpeg and stopped before the cases. A later test-shim
selector failed to inject quit/reset; its seven successful cases were retained
with identities and the missing two were rerun after correcting only the
harness. The final three UI-active cases ran separately. Earlier failures and
resource receipts were preserved; no product code was changed to make those
harness failures pass.
