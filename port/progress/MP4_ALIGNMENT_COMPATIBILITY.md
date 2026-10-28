# MP4 import: legacy alignment and bounded container validation

Issue #407 reports `Incomplete/invalid movie: movies/ROP.mp4` from the native
Linux Python importer. An owned 143,102,000-byte ROP export reproduces that
message: its complete top-level boxes end at byte 143,101,988, followed by twelve
CD fill bytes to a 16-byte boundary. Both preceding Python and C++ validators
reject it. This is a matching local reproduction, not the reporter's input.

Both validators now accept only this conservative legacy tail: 1–15 bytes,
all CD, ending on a 16-byte boundary, after complete `moov` and `mdat` boxes.
They do not search past a malformed box or accept arbitrary trailing junk.
Imported movie bytes are preserved; this does not reencode or edit the video.

The C++ sibling also needs `box <= remaining`, a 16-byte minimum for extended
headers, and bounded iteration. Its preceding extended-size arithmetic can wrap
the offset back to zero and repeatedly reread the same boxes. Both consumers
now share the existing Linux limit of 4096 top-level boxes; the final box may end
exactly at EOF or immediately before the permitted alignment tail.

## Verification

- `python port/tools/test_mp4_alignment.py` compiles and runs the actual C++
  function and executes the Python consumer. The 79 generated controls include
  plain/aligned movies, every possible short padding length, malformed tails,
  missing components, truncated/extended boxes, wraparound and 4096 boundaries.
  `--owned <movie>` adds a seek-only check of an existing movie without copying
  its contents. With the owned ROP, both backends pass 80 controls.
- The original Python consumer fails the aligned fixture. The original C++
  consumer rejects that owned ROP and loops on the generated wrap case; a
  private negative control terminates only its own process after 0.4 seconds.
  The corrected function rejects that same wrap case promptly.
- `test_linux_movie_import.py` exercises the real CLI producer and transaction
  with plain, XOR, mixed and padded sources; malformed outputs retain old HD
  and save bytes. Three test methods pass with the additional padded variants.
- `test_linux_native_import.py`: seven tests pass with the Windows symlink case
  skipped. Actual Linux symlink behavior requires the Linux run.

The small native probe ran alone at below-normal priority inside an owned
512 MiB Windows job. Peak job commit was 61.38 MiB and observed available system
RAM remained at least 6.265 GiB. Initial fixture-extraction failures were corrected
and excluded from product results. No GPU, VM, full reimport or media decoding
is needed for these tests.

This validates container boundaries, not every codec payload or physical-device
decoder. A full launcher/package build remains part of delivery validation.
