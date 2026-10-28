# Launcher startup failure boundaries

The Windows/Android native launcher previously returned 1 without a diagnostic
when SDL initialization or window creation failed. It also ignored both ImGui
backend initialization results. Android can finish the Activity after native
main returns without killing the process; absence of a process-exit record does
not establish successful initialization.

All four existing SDL/GL failure boundaries now use the same staged error path.
It copies the SDL detail into a bounded stack buffer before logging, showing a
best-effort error dialog and cleaning up. It records stderr and SDL logging
before SDL_Quit; a failed dialog is logged separately. Empty backend errors say
"no SDL error detail" rather than reporting a stale unrelated SDL error.

After successful make-current, the launcher checks the actual current context
and a nonempty GL_VERSION string before entering ImGui. The bundled OpenGL
backend parses that string without a NULL check when its numeric version query
returns zero. No GL query occurs when the context check fails. This is an
explicit failure, not a renderer/profile downgrade or retry loop.

ImGui backend results are checked. Failure destroys only successfully initialized
backends, the existing ImGui context, RetroAchievements UI, GL context and window
in dependency order. The bundled OpenGL backend's false return occurs before its
backend allocation; the current SDL backend returns true. Defensive false-return
checks do not establish a reproduced failure in either reporter's environment.

## Bounded verification

`python port/tools/test_launcher_startup.py` compiles and executes the actual
startup sections and failure helper with injected API boundaries. Baseline source
at ccb2695 fails for the two silent exits and both ignored backend results. The
preflight tests also failed before implementing the current-context/version
checks. The change passes 40 cases: desktop GL and Android GLES paths, success
and nine injected failures, each with a working or unavailable error dialog.
Checks cover missing current context, NULL/empty version, original error
preservation despite log/dialog/cleanup error clobbering, no downstream calls,
no shutdown of an uninitialized backend and one-time partial cleanup.

`test_launcher_gl_startup.py` retains its three existing GL bootstrap controls.
`test_android_launcher_ready.py` verifies unchanged launcher readiness contracts.
The changed whole translation unit is compiled with the cached Windows i686 and
Android arm64 NDK/API21 toolchains; this is compilation, not an APK/device run.
Linux's packaged launcher uses its separate Python frontend and is unchanged.

No Java, manifest, game source, GLES3 requirement, published package or protected
installation is changed. These checks do not prove an S22/Moto fix, prevent a
vendor driver fault inside a GL/SDL call, or catch arbitrary allocation failures.
