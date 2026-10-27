# Linux64 movie streaming - 2026-10-27

Branch: `fix/linux64-movie-streaming`; owner: `codex-linux64-streaming`.
Queue task: `optiport-linux64-streaming`. Base: `43987205a`.

The Linux64 player retained a non-null audio stream after decoder EOF. Once the
PCM queue drained, its clock stopped while every later video frame slept its
whole offset from that fixed position. A production-expression regression failed
before this change; the corrected production clock simulates 300 frames / 30 fps
in 9.967 seconds of scheduled waits, including a video with zero PCM samples.
At EOF the last PCM clock anchors the wall clock, preserving queued audio time.

FFprobe and both FFmpeg decoders now use direct `posix_spawnp` argv, owned child
PIDs and close-on-exec nonblocking pipes. Metadata, partial video frames and PCM
are read incrementally while events continue to be serviced. PCM fragments are
retained until a complete stereo sample frame can be submitted. Producer work
per iteration is bounded; there is no deadline that rejects a slow valid codec.
Cancellation closes pipes, kills only the owned direct children and reaps them;
there is no shell or `pclose` waiting for a decoder to finish its movie. Explicit
skip/quit consumes the scene even before an upload. Native fallback remains on
launch, allocation and initial upload failure.

Verification:
- `python port/tools/test_linux64_movie_streaming.py`: PASS on Windows LLVM;
  extracted production clock, audio EOF with queued PCM, zero PCM / 300 frames,
  and no audio device. Baseline clock regression observed failing first.
- `python port/tools/test_linux64_movie_streaming.py --emit-posix build/linux64-streaming/streaming.c`
  emits a standalone harness using production process/read/close/clock helpers.
- POSIX harness crosscompiles with llvm-mingw clang, `--target=x86_64-linux-gnu`,
  `--sysroot=D:/01Claude/ReKHCoM-build/linux/amd64/sysroot`, `-fuse-ld=lld`,
  `-Wall -Wextra -Werror`. Binary: `build/linux64-streaming/streaming`.
  Coordinator runs it serially in the Linux VM; runtime result pending here.
- Actual `port/linux64/linux64_host.c` syntax check with the Linux sysroot and
  real SDL3 headers: PASS. One pre-existing stb_vorbis pointer warning remains.
- `git diff --check`: PASS.

VM reproduction command for the coordinator:
`python port/tools/linux_vm_test.py --share build/linux64-streaming --run '/root/share/streaming > /root/streaming.log 2>&1' --fetch /root/streaming.log --out build/linux64-streaming/vm-results`

The POSIX harness covers stalled/partial/EOF producer pipes, no-data event ticks,
cancellation, idempotent cleanup, exact child reaping, inherited writer isolation
and exec failure. No real-device audiovisual playback, package rebuild, installed
main modification, push or release is claimed.
