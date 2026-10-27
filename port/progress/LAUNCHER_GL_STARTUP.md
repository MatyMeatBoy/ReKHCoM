# Launcher: failed OpenGL context must stop before rendering

The shared desktop/Android launcher did not check `SDL_GL_CreateContext` or
`SDL_GL_MakeCurrent`. A failed context therefore entered ImGui's OpenGL backend
and rendering with no usable current context. This is a source-level failure
path independent of the guest game, SRAM, save states and movie playback.

The launcher now stops on either failure, preserves the SDL error in its log
and a message box, releases the achievement client, any created GL context and
the window, then quits SDL with exit1. The successful context path and optional
swap-interval setting are unchanged.

`python port/tools/test_launcher_gl_startup.py` extracts and compiles the actual
production bootstrap and cleanup function, using controlled SDL/achievement
calls. On main `db4c0a8cb`, its context-failure case returns97: bootstrap returns
success, calls MakeCurrent with null, and continues to the downstream GL call.
The guarded consumer passes context failure, activation failure and success,
checking no downstream GL work after failure and one diagnostic/cleanup.
The complete Windows launcher translation unit also passes syntax checking
against the existing SDL3 headers, and the Android arm64 launcher passes the
same check with the existing NDKr30 and `LAUNCHER_ANDROID`/GLES3 definitions.
This CPU harness does not prove a particular
GPU driver or Android device creates/fails a context.

## Report374 remains separate and open

The reporter's [new comment](https://github.com/MatyMeatBoy/ReKHCoM/issues/374#issuecomment-6451357179)
says Android16, pre1 black screen then immediate exit, while v1.9.14 opens but
crashes after the Jafar sequence. Since v1.9.14 the launcher changed from a
Java Activity to SDLActivity loading SDL3 and the native GLES3 launcher; this
narrows investigation to bootstrap for the startup symptom, but does not
establish its failing call. There is no device log or confirmed cause yet.

Read-only inspection of the exact pre1 APK SHA256
`dca49ed80688b5e42624190660ef6c9028c11d45c55b7a499dbeb46ac868ca27`
finds 16KiB PT_LOAD alignment and matching offset/address congruence in all
three arm64 shared libraries. Android16 alone is not evidence of a 16KiB ELF
alignment failure. The current change is not presented as a confirmed Moto
fix. No APK, SDK or emulator was downloaded and no save was modified.

Post-Jafar on that old version remains a different report. Samsung report83
confirms progress past Jafar with the later build, which does not confirm the
Moto's runtime, phase or result. This guard does not change that battle.
