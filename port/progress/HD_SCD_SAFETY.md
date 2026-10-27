# SCD import: zero-progress auxiliary records

Baseline db4c0a8cb147a22b31351aa99c11608c72fbe35c, 2026-10-27. Follow-up to HD import investigation; this is an independently reproduced parser defect, not a confirmed explanation for #336/Bzshizzle's Steam Machine report.

A generated 256-byte SCD declaring aux=2147483647 and an auxiliary record length of zero repeatedly parsed the same offset. Both actual consumers (C++ ScdToOgg and native Python khhd_scd.convert) exceeded a two-second subprocess timeout. With aux=2 the same malformed record was incorrectly accepted by both. An empty Ogg output also mutated the C++ caller output on failure; testing it with no preallocated vector entered a process failure/timeout on this Windows host, so no specific device-crash diagnosis is inferred.

Both parsers now bound auxiliary count by bytes available for the minimum eight-byte record, require positive record progress of at least eight bytes, and validate each record against the actual buffer. Table/entry/header/seek/payload bounds are checked before access. C++ checks four Ogg signature bytes exist and only publishes the output buffer after successful conversion. Page granule scanning stops at incomplete tables/bodies. No arbitrary file-size or record-count cap is imposed.

Verification uses only generated owned bytes:

- test_hd_scd_safety.py:14 scenarios through the production C++ function and Python consumer, with valid auxiliary-record and encrypted Vorbis controls, malformed pointers/negative lengths/header/seek bounds, empty output, zero/huge auxiliary records and truncation.
- The same test invokes actual native transaction -> source_producer -> khhd_import subprocess -> package reader -> SCD parser. Six synthetic movie files and a synthetic music package exercise the pipeline without FFmpeg. Invalid SCD fails promptly, preserves existing HD/save bytes and removes staging/lock; the valid control commits Title.ogg and keeps the previous HD backup.
- Existing package safety, worker lifecycle, media coordinator and native transaction regressions are retained. Windows launcher syntax is checked with the installed LLVM MinGW x86/x64 compilers; no complete platform build, SDK/VM download, private game assets or installation access is needed.

Still unverified for the reporter: exact Proton build, import options, last visible phase, actual selected files and Media Foundation behavior. v2.0's missing native launcher is a separate packaging limitation; the published v2.1 archive contains the basic native entry. SD versus SSD alone does not select a failure mechanism. A useful next device test records the last import status/log and checks one selected source file with the same consumer; a clean native terminal run should be distinguished from Windows/Proton. This change does not resolve an external decoder call that never returns.
