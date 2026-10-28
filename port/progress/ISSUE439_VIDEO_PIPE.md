# Issue439: Linux64 HD video pipe readiness

The reporter confirmed public2.2, full-speed60fps gameplay and the launcher's “Just the basics” import. That button selects which resources to import; it does not imply a lower video resolution. The reporter's camera attachment was not inspectable with the available browser/media tools; no diagnosis is attributed to its unseen pixels.

Linux64 reads rawRGBA frames from FFmpeg through a nonblocking pipe. After a temporarily empty read, sleeping1ms each time multiplies scheduler latency across one large frame. A1080p frame is8294400bytes, much larger than the measured65536-byte pipe. Readiness polling wakes immediately when the next fragment arrives, with a requested1ms timeout so the outer loop continues servicing input and PCM. Scheduler overrun remains possible; this is not a hard real-time guarantee.

The change is limited to this empty-video-read wait. Encoded media, decoded size/format, frame order, audio clock, PCM submission, pause/skip and the32-bit backend are unchanged.

## Verification

CPU regression extracts the actual production consumer: baseline failed with449 artificial sleeps; fixed passes17 positive/empty/EOF/event/audio cases. Pause/input/language/clock contracts pass. The emitted realPOSIX producer test completes a1080p frame with409reads/164readiness waits and409event/audio pumps.

Native A/B uses the exact public Linux64 host b5fb7bb8119336b53c4f15f8dd07d0e0c9815604769a1e0c4732f2d4f4992908 and fixed host f608239b7c9a5ea2a6b8b2504cc42f861b710127bcaadefd608af9c28efe37e4. Same generated2s1080p30 H264 stream plus AAC audio, three alternating runs per host, oneCPU/512MiB Debian snapshot, SDLdummy/software640x360 output. Source resolution is identical in the paired host comparison.

| Host | Present-return p50 per run | Present-return p95 per run |
|---|---|---|
| Public baseline |131.07,131.67,110.30ms|157.86,167.27,126.93ms|
| Readiness wait |33.13,33.18,33.29ms|45.98,45.53,43.87ms|

Every full run uploads60frames, records60distinct sampled-frame hashes in the same order and makes60presents. The encoded input hash stays31d650b19f693bfb0f47001a0b8a087fb3c3c5a699cf50d4dc9924dc64a71c8d. No quality reduction or re-encoding is part of the fix. Both hosts' movie PCM has at most one internal zero sample-frame; the cancel/resume case intentionally contains the pause gap, returns to all60frames, and confirmed skip ends after19matching prefix frames.

A separate native FFmpeg/actualMovieRead transport-only comparison, without SDL/audio/presentation, independently isolates the wait policy: three1080p runs sustain6.5–7.7frame transfers/s with fixed sleeps versus33–40 with readiness wakeup, with the same60frames/497664000bytes/sampled hashes. Its720p cohort is a separate cost breakdown, not an equal-quality performance claim.

These are software-renderer/VM measurements, not physical SteamDeck/GamingMode GPU results. p95 still shows scheduling jitter. This does not promise perfect30fps or close439 without reporter confirmation. The pipeline test uses a synthetic bridge guest and generated media, not a ROM or story-playthrough. No SDK or dependency download was needed.

Local raw receipts: pruebas/issue439-linux-video-pacing/{transport-vm-attempt1,host-vm-attempt1}/, exact commands/input hashes, CSV update/present events, PCM, pause captures and compile logs. Host producer source873a5ce2b; subsequent rebase/documentation does not alter its compiled source files. Public stable assets remain immutable; root integrates439 with the current441/core assets before publishing a separate test.
