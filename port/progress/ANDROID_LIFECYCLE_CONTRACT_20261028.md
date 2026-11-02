# Android SDLActivity configuration contract

This change closes a manifest mismatch with the pinned SDL 3.4.18 template. Both launcher and game now handle locale, layoutDirection, grammaticalGender, fontScale and fontWeightAdjustment, in addition to the configuration changes already declared. Android can deliver these changes to the existing Activity instead of recreating SDLActivity in a live process.

SDLActivity's exact onCreate guard exits with status zero if mActivityCreated or mSDLMainFinished is already true and nativeAllowRecreateActivity is false. The default native setting is false. A fresh process starts with both flags false; the guard alone does not explain a first clean launch failing on Android16. No S22/Moto cause or reporter confirmation is established here.

Only the two configChanges declarations change product behavior. Launcher stays in the default process, GameActivity stays private in :game, orientations remain unchanged, and launchMode remains standard. No task/back-stack mode, native recreation allowance, GBA logic, native library, permissions or startup profile is changed. Arbitrary duplicate SDLActivity construction in a live process remains guarded; this patch does not claim to make SDL reentrant.

## Verification

- Base ccb2695f1f745c9973168f7d84eab73ff1931fa8. test_android_manifest_lifecycle.py first failed for exactly five missing flags in both Activities; after the patch both coverage and process/task contracts pass.
- Existing test_android_test_package.py: six tests pass. Source identity and task/process behavior remain separate from optional diagnostic profiles.
- Existing test_android_lifecycle.py compiles/runs the Java logger contracts: session/instance, process isolation, byte/queue/share bounds and failure preservation pass.
- A private JVM harness runs the guard block extracted verbatim from SDLActivity.java (SHA256 41e9a367d1b623d56735a2f6301dd2370ce1e43ecbb5e956ebb517202e2a200c), with nativeAllowRecreateActivity stubbed false: fresh creation continues, duplicate creation exits zero before its continuation, and finished-main creation exits zero. This is an exact Java guard test, not Android/native execution.
- Real aapt2 from build-tools35 compiles resources and links the product manifest against android-35/android.jar with minSdk21/target35. Both compiled config masks are 0x5000bff4, checked against SDK35 ActivityInfo constants. This proves schema/packaging, not installation or OEM behavior.
- The first private aapt audit failed its own dump regex/guessed mask; correcting those against actual output and SDK constants passed without another product change.

Evidence and reproducible private fixtures: pruebas/android-launcher-lifecycle-contract/ at the workspace root. API34 framework A/B runtime is prepared separately and its receipt must be consulted for execution status; it uses an isolated Java fixture with the exact guard and a native-setting stub, no product native libraries or ROM. It cannot certify full SDL video/GPU, game execution or Android16 hardware.

The earlier CONFIG_ASSETS_PATHS recreation after installation is already documented in OPTIPORT_V21.md: its first attempt did not reach its postcondition; a later stabilized run passed. That environmental incident is not a new universal crash finding and is not evidence for the S22 report. These five manifest flags do not claim to suppress every possible recreation (including explicit recreate, process death, or other configuration classes).

References: [Android configuration handling](https://developer.android.com/guide/topics/resources/runtime-changes.html), pinned SDL android-project/app/src/main/AndroidManifest.xml and SDLActivity.java; no SDK download or upgrade.
