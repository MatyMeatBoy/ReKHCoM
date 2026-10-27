# HD import package and worker failure safety

Baseline: 8d495811e, 2026-10-27. This fixes verified robustness failures; it does not establish the cause of the Windows/Proton crash in #336 or certify Steam Machine/Gaming Mode behavior.

A generated 16-byte header (dlen=-1, rcount=0, clen=-1) made production ReadAsset convert a negative length to size_t and throw; running it on a worker terminated the process. Valid raw20byte and zero-byte controls returned success and failure respectively. ReadAsset was byte-identical in tagsv2.0/v2.1 and baseline. A separate failing test of the real Parallel function terminated on a callback exception instead of joining. The Python native-import reader also accepted negative lengths.

ReadAsset now checks signed fields, offsets and real stream bounds before stored-data allocation. The existing stb decoder grows with actual decoded bytes, then its output must match the declared size; it never preallocates the untrusted decoded size. No arbitrary container-size cap is imposed. Rejected input preserves the caller output. The Python reader applies equivalent bounds and decoded-length checks, including remastered entries.

Parallel retains the first exception, requests cancellation, joins every started child and releases COM before rethrowing to the owner. The import/compression entry reports a named error and clears the running latch; thread creation rejection has the same terminal behavior. Scoped COM/Media Foundation cleanup also runs on exception. Existing journals and transaction staging retain their recovery rules. An external Media Foundation call which never returns remains outside this exception fix.

Checks, using generated owned bytes only:

- test_hd_package_safety.py: production C++ reader, real key expansion/stb zlib,16 encrypted/raw/compressed/empty/truncated/negative/mismatched cases; Python same corpus plus remastered and invalid offsets.
- test_hd_import_workers.py: real Parallel and worker launch/guard functions; actual std::thread callback failure, joining/COM cleanup, diagnostic/done latch, allocation/system/unknown failures, injected OS thread-start rejection and success.
- test_hd_media_import.py: production coordinator/cache/cancel/copy/corrupt preservation.
- test_linux_native_import.py:6pass,1symlink permission skip on Windows.
- Complete Windows launcher syntax checked with existing LLVM MinGW x86/x64 compilers. No SDK/VM/download or platform package build.

Published Linux2.1 archive SHA25625b4ef468100ea9d2e2154d177a22161fae726bfca54079a59d4ef5b602bbc2b was independently inspected and contains ReKHCoM Launcher.sh and launcher/rekhcom_launcher.py. That is packaging evidence for the basic native entry, separate from this source fix and reporter confirmation.

Review follow-up: completion is published exactly once by GuardImport, after the producer has returned/unwound, released COM/MF and finished error reporting. Run (including its nested music/sprite call) and Compress never write the running latch. Launch/preparation rejection publishes once only when no guarded worker exists. The worker fixture now executes the actual production Compress with a barrier inside MFShutdown on both success and failure; the latch must remain running while cleanup is blocked. At the terminal store the fixture immediately simulates a new job, which must remain running after the old owner returns. This failed against the initial PR head (exit20) and passes after centralizing completion. The synchronous Windows --hd-import/--hd-compress callers also use GuardImport and return failure when canceled; Android keeps its separate existing stubs. The separate export namespace uses its own sRunning flag and is outside the HD job latch.
