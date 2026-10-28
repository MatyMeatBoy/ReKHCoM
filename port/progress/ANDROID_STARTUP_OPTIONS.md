# Optional diagnostic launcher profiles

Opt-in builder switch: `build_android_diagnostics.py --compat-options`. This emits a separate `ReKHCoM-v2.2-test412-options.apk`, never replaces public test412 or stable2.2.

The plain Java diagnostic entry offers the default launcher, controller compatibility (HIDAPI off), and Java-only report tools. HIDAPI-off means SDL_JOYSTICK_HIDAPI=0, not disabling all controllers: Android controller detection and touch remain active. The game runs in a separate process and does not inherit this launcher profile. No Vulkan/software-renderer/audio workaround is offered because the launcher requires GLES3 and does not initialize audio.

Each launcher test runs in its own `:startup_profile` process. The Java entry validates the allowlisted profile name without setting the parent environment. The child captures its exact original libc value (including absent/empty) and applies the profile before SDLActivity.onCreate loads SDL. Unknown profile names are rejected by the Java entry; if an unexpected name or environment failure reaches the nonexported child, it explicitly records rejection, restores the original value and continues with the original default through normal Activity lifecycle. No deliberate exception is used as a fallback.

The native getArguments callback reads the effective SDL hint and records match status before SDLmain. A mismatch logs/displays failure; QA requires match=true before publication. Profile timestamps, process IDs, requested mode and effective hint are bounded to4KiB and included in the private report. No saved-game data or persistent profile preference is read/written.

When the test launcher finishes, after SDL destruction and environment restoration, only its verified own profile process is terminated. Neither the parent diagnostic process nor the game process is terminated. Transient onStop and nonfinishing recreation do not trigger this shutdown. This avoids unsupported reuse of SDLActivity3.4.18, whose mActivityCreated guard otherwise exits the process on a subsequent Activity creation. QA requires fresh launcher PIDs across default/off/default and an unchanged Java parent PID. Closing this isolated process intentionally can produce an exit-history entry; that alone is not evidence of a crash.

All eight native libraries and assets remain byte-identical to public2.2. This diagnostic build has no INTERNET permission and preserves application ID/versionCode/certificate. It is not a confirmed Android16/S22 fix. Normal stable builds do not include this path.
