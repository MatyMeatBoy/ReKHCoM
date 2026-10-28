# Optional diagnostic launcher profiles

Opt-in builder switch: `build_android_diagnostics.py --compat-options`. This emits a separate `ReKHCoM-v2.2-test412-options.apk`, never replaces public test412 or stable2.2.

The plain Java entry offers default launcher, controller compatibility (HIDAPI off), and existing Java-only report tools. HIDAPI-off means SDL_JOYSTICK_HIDAPI=0, not disabling all controllers. Android controller detection and touch remain active. The game runs in a different process and does not inherit this launcher's environment. No Vulkan/software-renderer/audio workaround is offered because the launcher requires GLES3 and does not initialize audio.

Profiles are session-only, no preference is saved. The Java gate rejects unknown profiles before starting SDL; environment setting errors leave the diagnostic screen. Exact original libc value (including absent/empty) is restored when the SDL launcher closes. The native getArguments callback reads the effective SDL hint and records match status before SDLmain. A mismatch logs/displays failure instead of throwing. QA must observe match=true before this candidate is ready. Attempt timestamps/mode/effective hint are bounded to4KiB and included in the private report; no saved game data is read.

All eight native libraries and assets remain byte-identical to public2.2. This diagnostic build still has no INTERNET permission and preserves application ID/versionCode/certificate. It is not a confirmed Android16/S22 fix. Default stable builds do not include this path.

Lifecycle refinement approved by root: each profile is an isolated :startup-profile process. Plain Java entry validates names only; child applies environment before SDLActivity.onCreate. On finishing, after SDL destruction and environment restoration, it terminates only its verified own profile process, never the parent or game. This avoids SDLActivity3.4.18 intentionally exiting when reused (mActivityCreated guard). Unknown/env failure inside the nonexported child explicitly logs rejection and uses original default with proper super lifecycle. QA requires fresh profile PIDs and unchanged Java parent PID, not unsupported SDL static resets.
