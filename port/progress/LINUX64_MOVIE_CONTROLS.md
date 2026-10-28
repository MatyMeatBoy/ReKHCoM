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
remembered1..5 game language. Automatic configuration plus `--language es` is
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
CPU checks pass. This is not a rendered screenshot or physical Deck test.

Runtime release checks remain necessary: confirm Continue/No/Back preserve
movie position and audio, Yes skips, FPS is visible only when enabled, and
renderer reset/quit leave the movie cleanly. Existing harness controls
`KHCOM_HDMOVIE_PAUSETEST=<seconds>` and
`KHCOM_HDMOVIE_PAUSE_ACTION=cancel` can walk the pause and confirmation windows;
they write `hdmovie_pause.bmp` and `hdmovie_skip_confirm.bmp` in the isolated
data directory. Missing-font and resized-window paths require those checks too.
