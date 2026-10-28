# Linux64 decoded-frame pipe progress

Issue421 reports slow HD intro playback on Steam Deck with v2.2-pre3. The
Linux64 movie consumer imposed a one-millisecond delay after every partial
video read, including successful reads. A decoded1920x1080 RGBA image occupies
8,294,400bytes; transport in4KiB fragments consequently inserted2024 waits
before a single frame was ready. This is a reproduced consumer defect, not a
measurement of that reporter's pipe size or physical Deck performance.

The consumer now waits only on an empty nonblocking read. Each fragment still
returns through the outer event/audio pump; EOF/error handling, audio clock,
frame presentation times and sample bytes are unchanged. Linux AArch64 reuses
the same host implementation. Native32, Windows and other decoder backends do
not use this fragment branch and need no equivalent patch.

`python port/tools/test_linux64_movie_fragments.py` extracts the production
consumer and runs17 cases:4–64KiB fragments,1080p/720p frames, interleaved empty
reads, EOF mid-frame and complete reads. Before the fix it fails on2024
artificial waits; afterward positive progress has no imposed sleep, while
empty reads still yield and events/audio are pumped for every read. Existing
`test_linux64_movie_streaming.py` covers the production clock and can emit the
POSIX producer/pipe cancellation harness for native Linux execution.

Physical Steam Deck video/audio pacing remains a reporter/device confirmation.
The separate missing pause/confirmation UI and FPS display observations in421
are not fixed by this transport change.
