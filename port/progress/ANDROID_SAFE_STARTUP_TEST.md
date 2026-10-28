# Current Android safe-startup test

The optional test enters a Java Activity without loading SDL/ImGui/GLES. Play
validates an existing private ROM and starts the existing GameActivity in:game;
SAF/atomic RomImport remains available, without reimporting a valid installed ROM.
The graphical launcher is optional and isolated in:launcher. Installed native
libraries determine the ABI; no32-bit fallback is forced on a64-bit-only device.

Native code/assets come from separately recorded inputs, not the old test412
native APK. The offline variant builder preserves application ID, signing
certificate and versionCode for updates without clearing data. It changes only
the test manifest, preserving the normal release. Configuration handling is the
union of the app's flags and its actual SDL3.4.18 manifest template; task launch
modes are not copied blindly. Build/badging do not certify install/runtime.

Safe launches have nonce/PID/generation identities and finite stage names. The
actual SDL nativeRunMain return is retained on its worker thread, including1
without a process crash. Launcher444 errors are also saved before cleanup to a
small app-private file: logcat alone is not a retained report. The opt-in report
contains bounded own-native log tails and lifecycle/stage records, not ROMs,
saves, settings, global logcat or other apps' exits. Missing records are not proof
of success. A return0 alone is not a gameplay/device health certificate.

The Java entry waits for the previous owned SDL process to disappear using a
bounded worker-only check. Retirement applies only to its own process name/PID,
the current attempt/Activity generation, finishing without configuration change,
after SDL cleanup and a terminal captured SDL thread. A live thread is not killed;
stale cleanup A cannot retire already-claimed B. This is not a rewrite of SDL's
same-process recreation internals; the entry avoids overlapping SDL launches and
the report keeps ambiguous lifecycle outcomes observable.
The Play latch stays busy until its child Activity result, including across saved
Activity state; initial resume and picker cancellation do not release it. All
actions and report text share one ScrollView for landscape accessibility.

The physical374 update reports API36 native SIGSEGVs in the older
:startup_profile child, not a proven common cause with Deck330. The exact own
process allowlist retains that historical name. Before replacing report.zip, the
collector preserves its prior bytes once; rolling share snapshots do not remove
that cache backup. Cache retention is not a substitute for exporting the original
ZIP before an update. The old test412 builder fails before staging rather than
combining current Java diagnostics with obsolete native libraries.

Validation includes portable ownership/journal/installed-ROM controls, actual
StartupSdlActivity callbacks with explicit JVM framework/native-call fixtures,
real Android35+SDL Java compilation, manifest negatives under python-O, and the
actual C++ startup failure paths with persisted original errors. Device/emulator
install, Java-first UI, picker, repeat fresh PID and native-return/report tests
remain a separate gate. No Moto/S22 cause or universal crash fix is claimed.
