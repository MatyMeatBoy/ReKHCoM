# Android startup diagnostics implementation plan

> For agentic workers: execute these bounded tasks with the executing-plans workflow. Root has selected and authorized this design; no additional user confirmation is needed.

**Goal:** Optional test412 APK that opens a small Java screen before SDL and lets the affected reporter inspect/share their own prior process-exit record.

**Architecture:** Variant-only Activity/collector/provider sources and a separate packaging tool. Normal Android sources and its launcher manifest stay unchanged. Extract all eight ELF libraries and all assets from the SHA-pinned public2.2 APK; rebuild Java/manifest only, preserving applicationId, signing certificate and versionCode102020099 for reversible installation. Root reviews before a separate test412 prerelease.

**Tech stack:** Existing Android SDK35, Java8 language target, offline Gradle8.12/AGP8.7.3 cache; built-in Android widgets, ActivityManager/ApplicationExitInfo and read-only ContentProvider. No SDK/NDK download, native rebuild, new runtime dependency, ROM or network collection.

## Constraints and sources

- Existing normal release APK SHA256: bd7999c234669ac88dfd5ec22ae3ccddd95db011eda0a1608fb8526819e19a4e. Never overwrite public assets.
- applicationId com.rekhcom.android, same official certificate, versionCode102020099; distinct diagnostic versionName.
- API30 provides own-UID historical records in a limited ring buffer; records may be absent. Query only this package,0pid,max8, and retain only main/:game process names.
- API31 native traces are protobuf tombstones, not UTF-8 text. Keep exact bounded .pb bytes; ANR traces are separate .txt files. Missing/expired/denied/oversized traces get an explicit status, never a fabricated trace or root cause.
- [ActivityManager primary docs](https://developer.android.com/reference/android/app/ActivityManager#getHistoricalProcessExitReasons(java.lang.String,int,int)) and [ApplicationExitInfo primary docs](https://developer.android.com/reference/android/app/ApplicationExitInfo#getTraceInputStream()). Same UID needs no DUMP permission; other UIDs are excluded.
- UI: Open launcher, Copy summary, Share report, Refresh; plain Java Activity with hardware acceleration disabled. Do not reference/load SDL until explicit Open launcher action. Diagnostic manifest removes network permission; no global file/log permissions.
- Collector runs off the UI thread. Bound metadata/summary to64KiB and retained trace data to4MiB total. Sharing is explicit through a read-only, non-exported, grantable provider limited to one report ZIP. No saves, ROMs, tokens, configs or other apps' reports collected.

## Tasks and acceptance

- [ ] Write failing pure-Java tests for bounded stream copy, summary truncation, safe ZIP filenames and byte-exact native traces; then implement the small report helper.
- [ ] Implement API-isolated collector and Java Activity/provider under `port/android/diagnostics/`, with API<30/no-history/error fallback. Validate own-UID query and no SDL dependency before first UI.
- [ ] Add `port/tools/build_android_diagnostics.py`: pin/CRC-check publicAPK; reuse existing Java/Gradle wrapper/cache offline; exact eight native/asset hashes before and after packaging; verify package identity/version/certificate. Normal builder untouched.
- [ ] CPU/compile/package tests: Java helper fixtures, manifest/permissions/provider scope, no native load in entry Activity; APKasset+ELF hash parity and same versionCode/certificate.
- [ ] Existing API34 read-only AVD: stable install → private-data sentinel → diagnostic same-version update → Java UI + copy/share + optional SDL action → stable same-version return. Sentinel must survive all transitions; no downgrade flags or uninstall/clear. Validate no SDL .so in process maps before explicit action.
- [ ] Preserve all evidence, including environment failures. Native-crash history absent on emulator is a limitation, not proof S22 fixed. Small PR and diagnosticAPK await root review/publication approval.
