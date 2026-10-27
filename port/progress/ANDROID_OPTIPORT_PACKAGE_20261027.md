# Isolated playable Android OptiPort test APK — 2026-10-27

Task `android-optiport-package`, owner `codex-android-test`.

Build command for coordinator, after integrating the desired OptiPort changes:

```powershell
python port/tools/build_android.py --test-build --test-version 2.1-nightly.20261027.1 --regions eu us jp --jobs 2
```

The install ID is `com.rekhcom.android.optiport`, separate from stable
`com.rekhcom.android` and automation `com.rekhcom.android.smoke`. Label:
`ReKHCoM OptiPort 2.1-nightly.20261027.1`. Staging uses `build/android-app-optiport`;
APK output and build-manifest names also carry `-optiport`. Existing native build
folders are reused: serialize native builds as usual. The normal Gradle
`assembleDebug` and existing debug signer remain unchanged.

No battle fixture, scripted keys, screenshot or auto-quit metadata is inserted.
GameActivity remains non-exported. Both 32-bit cores and the default 64-bit host
are included, with all EU/US/JP regions unless explicitly narrowed by the caller.
Only `applicationId` changes: Java namespace and fully-qualified activity names
remain `com.rekhcom.android`. Launcher uses `GameActivity.class`; native launcher
JNI resolves the activity object's actual class, not the application ID.

`--test-version` labels the APK only. Native `REKHCOM_VERSION` is unchanged and
its value is recorded separately as `source_version` in the build manifest. If
the coordinator wants native logs/header to show the nightly version too, that
must be an explicit source/version change before compilation. This script does
not silently relabel native code.

Installing this package creates a new app sandbox. To use an existing manual
save, export it through the stable launcher's save export, then import the
normal SRAM save through the OptiPort launcher's save importer, choosing the
same region and bank. The existing importer validates region/header/checksums,
accepts 32/64 KiB SRAM (and an unambiguous repeated 128 KiB container), and retains
a dated backup when replacing an existing destination. Quick states are not
manual SRAM saves. ROM/HD asset import also runs independently in the new app.
No saves, ROMs, configs or personal logs are packaged into the APK.

Validation: configuration helper tests failed before implementation and pass
now. They verify stable/smoke identities, isolated nightly identity/version code,
manifest activity binding/non-export, absence of SDL_ENV autoplay metadata and
rejection of conflicting/unsafe arguments. The existing production Java manual
SRAM importer also passes its 48 checks across EU/US/JP. `--help` and `git diff --check` pass.
No Gradle/native build, device install, device playback, upload or release was
performed in this worker task. Coordinator must inspect the built manifest and
record source SHA/hash/device results when producing the test APK.
