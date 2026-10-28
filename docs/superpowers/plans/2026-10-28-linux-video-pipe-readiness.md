# Linux HD video pipe-readiness plan

Goal: replace repeated fixed1ms video-pipe empty sleeps with bounded fd-readiness waiting while preserving encoded media, source resolution, audio clock, frame order, event pumping and pause/skip behavior.

Evidence: actualMovieRead + nativeFFmpeg transport in existing512MiB/1CPU Debian snapshot, same H2641080p30 input SHA7b9cfb12ce57b6c382be82f49e4cf9772baea0173e15184df7c858dba123fb98, three alternating A/B pairs. Baseline transport6.5–7.7frames/s versus readiness-poll33–40; same60frames/497664000bytes/sampledhash. This isolates waitpolicy, not GPU, physicalDeck, audio or actualpresentationFPS. Separate720cohort is not part of equal-quality improvement claim.

- [x] RED actualconsumer regression forbidsfixedsleep onEAGAIN while preservingpositiveprogress/EOF/eventaudio pumping.
- [x] MinimalLinux64 helper poll(POLLIN,1ms max), usedonlyvideoEAGAIN. Noencoding/asset/importerchange.
- [ ] GREEN CPU tests and realPOSIXfragmentproducer; actualhost compile fromclean source.
- [ ] RealFFmpeg same encodedfile hash, actualbaseline/fixedhost audio/video/pause/skip cases and presentation timestamps; distinguishsoftwarebackend/VM fromDeck.
- [ ] Small PR with rawcomparabletiming, source/binary/mediahashes andhardwarelimit. Reporterissue remains open untilconfirmation; rootintegrates/buildpublishes.
