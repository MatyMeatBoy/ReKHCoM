# Android picker: ROM ZIP classification and atomic replacement

The picker previously treated every `PK0304` file as an HD pack. A ZIP containing
a ROM therefore replaced `ReKHCoM-HD.zip`, never installed a region ROM, and left
Play unavailable. An extracted production Activity method reproduced this with
a generated ZIP containing a deliberately invalid 16-byte `game.gba`: it reported
HD success and replaced the previous pack instead of rejecting the ROM.

The picker now inspects the bounded ZIP inventory before choosing a destination.
A single ROM entry must have the original exact 32 MiB size, matching CRC and one
of the unchanged EU/US/JP SHA-1 values. Raw dumps use the same size/hash whitelist.
ROM and HD entries cannot be mixed; multiple ROM candidates, unsupported archives,
unsafe names and truncated files are rejected. Entry names are never output paths.
The only destinations are a recognized region slot or `ReKHCoM-HD.zip`.

Input staging is capped at 8 GiB for HD archives and 32 MiB for raw ROMs. ZIPs are
capped at 16,384 entries and 8 GiB aggregate declared uncompressed bytes; a ROM ZIP
has a tighter 64 MiB aggregate limit. ROM decompression also enforces the actual
32 MiB limit while streaming. Fixed 64 KiB copy buffers avoid loading the archive
into memory. Temporary files and input/ZIP streams close before committing.
Android's API-21 `Os.rename` atomically replaces the destination on the app's
filesystem, without first deleting a valid previous file.

HD classification accepts the native installer's supported media paths. It does
not decode all media or CRC-scan the entire HD payload in the picker. Deep pack
validation and media-generation activation remain with the existing native
consumer; no extra full-pack decompression pass was added. Import failure and
native media-activation failure are distinct boundaries.

## Verification

- `python port/tools/test_android_rom_import.py`: 49 generated JVM checks against
  the production helper, including limits, ambiguous/mixed ZIPs, unsafe paths,
  truncated inputs, short reads, copy/close/atomic-commit failures and exact
  preservation of previous ROM slots, pack, installed HD, configuration and save.
- With `--rom <owned-clean-dump>`, 59 checks: the actual EU dump imports both raw
  and zipped, its SHA matches the installed output, and a corrupted central CRC
  is rejected even when ROM bytes/hash remain valid. Payloads stay private.
- `python port/tools/test_android_launcher_ready.py` executes the production
  Android status parser, completion poll, `ScanInstall`, `KeepSelection` and
  `GameReady` with a small JNI-string adapter. All 24 installed-region/selection
  combinations pass. `--files <owned-helper-output>` additionally verifies the
  installed EU ROM enables Play after the completion serial, preserves it after
  an import error, and avoids rehashing an unchanged serial. Region fallback was
  already correct; native selection logic is unchanged.
- `python port/tools/test_hd_media_pack.py`: the actual miniz/native installer
  retains the previous active media generation and save on incomplete transfer,
  while allowing complete same-size replacements. This does not claim arbitrary
  legacy HD files are transactional.
- Actual Activity, helper and bundled SDL Java sources compile against the local
  Android 35 SDK with Java 8 targets. No APK/device/JNI-ABI or physical storage
  test was performed. US/JP valid payloads were not available for filesystem QA;
  their unchanged whitelist entries and selection masks are covered separately.

The Retroid report's selected file format is still unconfirmed, so this concrete
workflow bug is not attributed to that reporter yet. A Samsung crash before the
launcher opens is an earlier phase and is not addressed by this patch.
