# Android diagnostic startup options plan

Goal: an optional session-only HIDAPI-off launcher path beside unchanged default and the existing Java-only report screen. Root approved this bounded design; no physical device fix claimed.
Architecture: plain Java entry validates profile names; an isolated nonexported SDL subclass process applies the allowlisted environment profile before SDLActivity.onCreate, capturing and restoring exact prior libc environment (including absent) with android.system.Os. getArguments reads actual SDL nativeGetHint and records effective value before SDLmain; mismatch is visible and logged, never throws a new crash. Normal launcher Java/native8ELFs/assets untouched. Builder opt-in --compat-options adds subclass and labels separate test412-options APK; default existing diagnostic output remains unchanged.

- [ ] Pure Java profile contract RED/GREEN: default restoration null/empty/custom, HIDAPI0, sequential off/default, unknown rejected.
- [ ] Diagnostic UI buttons/default labels and bounded profile attempt evidence; subtype registered only in optional variant. Record effective hint with SDL JNI, never null nativeSetenv.
- [ ] Offline packaging exact2.2 native/assets, sameID/code/cert, new APK/hash. Normal and originaltest412 stable/public artifacts immutable.
- [ ] API34 actual Java screen noSDL, touch default/off/default launches, actual hint log/evidence and unknown refusal, no sticky environment. Preserve previous roundtrip/privacy proof where code unchanged; verify current entry/share.
- [ ] PR with source/hash/proof and explicit physical-device limitations. Root review/integration/publication only.

Lifecycle refinement approved by root: each profile is an isolated :startup_profile process. Plain Java entry validates names only; child applies environment before SDLActivity.onCreate. On finishing, after SDL destruction and environment restoration, it terminates only its verified own profile process, never the parent or game. This avoids SDLActivity3.4.18 intentionally exiting when reused (mActivityCreated guard). Unknown/env failure inside the nonexported child explicitly logs rejection and uses original default with proper super lifecycle. QA requires fresh profile PIDs and unchanged Java parent PID, not unsupported SDL static resets.
