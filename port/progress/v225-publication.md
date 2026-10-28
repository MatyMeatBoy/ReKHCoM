# v2.2.5 publication record

Published on 28 October 2026 as the [stable Latest release](https://github.com/MatyMeatBoy/ReKHCoM/releases/tag/v2.2.5).
The release tag resolves to `ea99f7ea49521d4509793d3c13b8dad6b214263b`.
Windows and Linux desktop/Steam Deck retain stable status; Android, Anbernic,
PS4, PS5, Vita, Switch and Web remain beta.

## Downloads and immutable identities

The release has ten assets: the eight packages below, the
[build/source and verification manifest](https://github.com/MatyMeatBoy/ReKHCoM/releases/download/v2.2.5/BUILD-SOURCES-v2.2.5.json),
and [SHA-256 checksums](https://github.com/MatyMeatBoy/ReKHCoM/releases/download/v2.2.5/SHA256SUMS-v2.2.5.txt).
Previously delivered packages and test releases remain unchanged. No ROM is included.

| Platform | Package | SHA-256 |
| --- | --- | --- |
| windows | [ReKHCoM-v2.2.5-windows-x86-x64.zip](https://github.com/MatyMeatBoy/ReKHCoM/releases/download/v2.2.5/ReKHCoM-v2.2.5-windows-x86-x64.zip) | `988b789d9ae1bce52c4c5771268bf07683e72a3c6359fc6a59c21da534f2d87e` |
| ps4 | [ReKHCoM-v2.2.5-ps4-beta.pkg](https://github.com/MatyMeatBoy/ReKHCoM/releases/download/v2.2.5/ReKHCoM-v2.2.5-ps4-beta.pkg) | `88a02210c171002770ba584c15166db3136312c707106504626abbe4a2ad25e2` |
| vita | [ReKHCoM-v2.2.5-vita-beta.vpk](https://github.com/MatyMeatBoy/ReKHCoM/releases/download/v2.2.5/ReKHCoM-v2.2.5-vita-beta.vpk) | `dcec3dfe616dc5298df6d30d026826914e39c7104f4feac3af6e437658e4acbf` |
| android | [ReKHCoM-v2.2.5-android-beta.apk](https://github.com/MatyMeatBoy/ReKHCoM/releases/download/v2.2.5/ReKHCoM-v2.2.5-android-beta.apk) | `6ddb671726a12caed4f65e696635a7672c444a849b0d16e9504bd638b104604e` |
| switch | [ReKHCoM-v2.2.5-switch-beta.nro](https://github.com/MatyMeatBoy/ReKHCoM/releases/download/v2.2.5/ReKHCoM-v2.2.5-switch-beta.nro) | `b304d6c557d2e826d56304b31cbea2473a426477d751bbf68b7c40a59610981b` |
| ps5 | [PPSA98511.zip](https://github.com/MatyMeatBoy/ReKHCoM/releases/download/v2.2.5/PPSA98511.zip) | `563ea35d35349ed91c5d5d5098c1a5476ed1acd1b94e6fb28c70ccbb34459139` |
| linux | [ReKHCoM-v2.2.5-linux-x86-x64-desktop-steamdeck.tar.gz](https://github.com/MatyMeatBoy/ReKHCoM/releases/download/v2.2.5/ReKHCoM-v2.2.5-linux-x86-x64-desktop-steamdeck.tar.gz) | `2c70860d565a1c12ccad32517cac33bdb5fde1c71d106201be9e8d24461a3194` |
| anbernic | [ReKHCoM-v2.2.5-linux-armhf-anbernic-beta.zip](https://github.com/MatyMeatBoy/ReKHCoM/releases/download/v2.2.5/ReKHCoM-v2.2.5-linux-armhf-anbernic-beta.zip) | `964afc9d7705256fdb05925243883332992377825029353dafdb4aeac90da4f1` |

Native packages were compiled from `f28b884ab2409753cea0cfae37784d5a3ed90f31`.
The Android official Java-first packaging uses
`17c385bddf8c44de9a9130068ac9c7a51d7eabf3`, retaining the verified native/guest bytes.
The manifest records these component identities separately from the release tag;
it does not claim every native package was rebuilt at the later Web fix.

## Web deployment

[Play Web v2.2.5 Beta](https://matymeatboy.github.io/ReKHCoM/).
The published Pages commit is
[`46e2224a4fd65d899193c53ec7141ce93826dfca`](https://github.com/MatyMeatBoy/ReKHCoM/commit/46e2224a4fd65d899193c53ec7141ce93826dfca).
Its 30 public files were checked against the closed publication inventory.

All three regional Web builds use the ABI correction at the release tag, with
`src/bos/bos_pc.c` recompiled per region. Unchanged cached objects from `17c385b`
were retained only after a strict recipe comparison; all three links passed with
fatal linker warnings enabled. The six actual Wasm/Node ABI controls and frontend
checks passed. Browser ROM startup and physical GPU behavior remain unverified.

## Delivery checks and limits

- All ten remote asset names, sizes and SHA-256 digests match the closed files.
  The actual download matcher selects exactly one Windows, Linux x64 and Android
  package; the Windows catalog filter excludes the PS5 ZIP.
- Anonymous downloads of the PS5 package and both manifests returned the expected
  bytes. The release is public, not a prerelease, and is Latest at publication.
- Windows: 16 packaged title/saved-map/door/room cases and seven launcher OpenGL
  pages passed across the scoped 32/64-bit controls. Saved-map HD checks used the
  packaged resources and native fallback where an external HD import was absent;
  they do not certify a complete proprietary HD import or audible-device pacing.
- Linux: 14 exact-archive VM controls passed, including keyboard without a pad,
  regional titles, video cancel/skip and return, nonzero PCM, importer data
  preservation and the private PulseAudio dependency. Physical Deck/Bazzite
  startup and frame pacing are not certified by those VM controls.
- Android: APK entry, signature, version, eight native ELFs, three guest images,
  216 asset members and archive integrity passed. Final-APK installation/runtime
  remain unverified; earlier reporter-confirmed font tests are separate evidence.
- Console packages and Anbernic passed build/static metadata checks. Physical
  console and Anbernic runtime remain unverified.

## Reports at publication

The fixes and final download instructions were posted before closing
[#439](https://github.com/MatyMeatBoy/ReKHCoM/issues/439),
[#441](https://github.com/MatyMeatBoy/ReKHCoM/issues/441),
[#449](https://github.com/MatyMeatBoy/ReKHCoM/issues/449) and
[#461](https://github.com/MatyMeatBoy/ReKHCoM/issues/461).
The earlier font-test reports
[#374](https://github.com/MatyMeatBoy/ReKHCoM/issues/374) and
[#412](https://github.com/MatyMeatBoy/ReKHCoM/issues/412) were closed after reporter confirmation.

Keep [#330](https://github.com/MatyMeatBoy/ReKHCoM/issues/330),
[#459](https://github.com/MatyMeatBoy/ReKHCoM/issues/459) and
[#460](https://github.com/MatyMeatBoy/ReKHCoM/issues/460) open for the remaining
Steam startup/audio evidence. In #459, the reporter says `--safe-startup` did not
help; Steam Linux Runtime 1.0 starts the game but HD movies do not play. The two
linked logs required access and could not be read; accessible copies were requested.
No cause is assigned from that result alone.

[#479](https://github.com/MatyMeatBoy/ReKHCoM/issues/479) tracks the missing
Quit Game option. Newly reported
[#480](https://github.com/MatyMeatBoy/ReKHCoM/issues/480) describes Android map
layer/spring problems in Halloween Town and some other rooms; it remains unresolved.
These records do not claim zero bugs or completion of all audio/art work.

## Next v2.3 priorities

1. Map-card HD coverage: the `99` total, `/` separator and missing small map-card
   illustrations, retaining the original frame and corner pixels.
2. Jiminy's Journal internal submenus, including the rounded right book edge.
3. Audio stutters when skipping the intro to the title screen and opening the
   Moogle shop submenu, followed by the broader continuity pass.
4. Restore the custom menu's complete Quit Game action, including HD mode, so it
   closes the application without reopening the launcher (#479).

These are pending work, not changes retroactively added to the immutable v2.2.5 packages.
