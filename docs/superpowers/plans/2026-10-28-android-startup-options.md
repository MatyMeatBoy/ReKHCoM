# Android diagnostic startup options plan

Goal: an optional session-only HIDAPI-off launcher path beside unchanged default and the existing Java-only report screen. Root approved this bounded design; no physical device fix claimed.
Architecture: plain Java diagnostic entry applies an allowlisted environment profile before launching the nonexported SDL subclass; captures and restores exact prior libc environment (including absent) with android.system.Os. getArguments reads actual SDL nativeGetHint and records effective value before SDLmain; mismatch is visible and logged, never throws a new crash. Normal launcher Java/native8ELFs/assets untouched. Builder opt-in --compat-options adds subclass and labels separate test412-options APK; default existing diagnostic output remains unchanged.

- [ ] Pure Java profile contract RED/GREEN: default restoration null/empty/custom, HIDAPI0, sequential off/default, unknown rejected.
- [ ] Diagnostic UI buttons/default labels and bounded profile attempt evidence; subtype registered only in optional variant. Record effective hint with SDL JNI, never null nativeSetenv.
- [ ] Offline packaging exact2.2 native/assets, sameID/code/cert, new APK/hash. Normal and originaltest412 stable/public artifacts immutable.
- [ ] API34 actual Java screen noSDL, touch default/off/default launches, actual hint log/evidence and unknown refusal, no sticky environment. Preserve previous roundtrip/privacy proof where code unchanged; verify current entry/share.
- [ ] PR with source/hash/proof and explicit physical-device limitations. Root review/integration/publication only.
