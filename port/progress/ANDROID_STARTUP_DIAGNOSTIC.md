# Optional Android startup diagnostic

This variant gathers evidence for startup failures such as #412. It does not
claim to fix S22 Ultra/Android16. Its first screen uses ordinary Java widgets
and does not load SDL or the native launcher until **Open launcher** is pressed.

Install it as an update, keeping existing app data. Open the launcher once; if
it closes, reopen the app and use **Share report** (ZIP) or **Copy summary**.
The screen can be useful without USB debugging. If the Java diagnostic screen
itself cannot open, an Android system crash report is still needed.

The report contains only this package's retained main/:game exit records.
Records can predate the current APK; timestamps/process names matter. Updates
and force-stop can add unrelated exit records. Android retains a limited
history and may omit/expire traces. An exit reason is not a verified root cause.
Native traces on Android12/API31+ are preserved as raw protobuf `.pb` files,
not decoded as text. No saves, ROMs, account files or global logcat are read.
Each Share action creates an immutable UUID snapshot and grants read access only
to that exact URI. Refresh never changes previously shared bytes. At most eight
snapshots are retained in a dedicated private cache directory (about32MiB of
trace data); an expired URI stops working and is never reused for another report.

The diagnostic APK requests no permissions and has networking disabled. Network
features of the normal launcher/game are therefore unavailable in this variant.
Return to normal by installing the public v2.2 APK as an update. Do not uninstall
or clear app data: both APKs retain applicationId `com.rekhcom.android`, certificate
SHA256 `3a5a1740e8171c82b505907da26f3039e2153aeb8d16c2c22477d52edf74ecdc`
and versionCode `102020099`; the diagnostic versionName is explicit.

## Packaging

Use the separate `port/tools/build_android_diagnostics.py` with the public
v2.2 APK, an existing generated Gradle wrapper/project and the existing Android
toolchain. It works offline without native compilation. The input APK is pinned
to SHA256 `bd7999c234669ac88dfd5ec22ae3ccddd95db011eda0a1608fb8526819e19a4e`.
All eight native libraries and every asset must remain byte-identical after
packaging; normal Android source/manifest/build paths are unchanged. The builder
also verifies signing identity, package/version, entry Activity and permissions.

## Bounded validation

Initial product source `b162c8954a4e494892b92204c1969c8d6750ca39` produced APK
SHA256 `9cc6e458b6780801ade3d745d1ace2e5cd91e9cc6714fd8b136801408c891e84`
for the complete installer roundtrip. The immutable-share refinement at product
source `4007948f030e63a6fa0a2e55d3ab92c15f3ef2c5` produced candidate SHA256
`28847519552dd22f904eb2645e7388a101214d1096d19e6796d201dca1363bae`.
Its225 native/asset/binary-manifest entries and install identity are identical,
so that installer proof remains applicable; new sharing/UI checks use the new APK:

- ROM-free JVM checks cover exact binary/empty copies, rejection of oversized
  traces, safe ZIP names and a valid UTF-8 byte cap. SDK35 Java compilation passes.
- Actual API34 read-only AVD: stable → diagnostic → stable updates with the same
  versionCode and no downgrade/uninstall/clear preserve a private-data sentinel.
- The Java screen opens before any SDL/launcher/guest library appears in the
  process maps. Native SDL/launcher libraries appear after explicit Open action.
- Copy action and Android share chooser run; no report is sent to another person.
  Two new UUID Shares around a Refresh preserve the first snapshot byte-for-byte
  while the second contains the updated ZIP. CPU checks enforce retention8,
  canonical/name rejection and eight non-read/write modes in the real provider.
  The provider denies an ungranted shell caller.
- A retained historical native :game trace from this QA environment exports in a
  valid ZIP as 286700 raw protobuf bytes. It is not a S22/current-build crash.
- First roundtrip was interrupted by an unrelated emulator System UI ANR; logs
  and image are preserved. The second complete roundtrip passes without that
  interruption. This verifies the diagnostic flow, not physical S22/Android16.

Local receipts/captures are in `pruebas/android412-v22-startup/`, including the
exact APK build receipt and `diagnostic-roundtrip-attempt2/RESULT.json` and
`snapshot-validation-attempt1/RESULT.json`.
Root review precedes any separate diagnostic prerelease. Never add/replace a
diagnostic APK in the immutable public v2.2 release.
