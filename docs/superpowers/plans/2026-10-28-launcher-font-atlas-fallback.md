# Launcher font atlas recovery

Five exact-build Moto tombstones establish an unbuilt ImFont (FontSize0,
ContainerAtlasNULL) reaching SetCurrentFont from NewFrame. A real ImGui probe
reproduces this state with a failing font merge. The particular OEM font is not
known. Do not change graphics drivers, ROMs or game rules to conceal this fault.

1. Keep a release-mode, actual ImGui regression for valid, invalid merged and
   absent fonts, partial/stale role pointers, and failure of both build attempts.
   Compare the old unchecked path with the guarded producer and real NewFrame.
2. After LoadFonts, before backend startup or either frame loop, explicitly
   build and validate atlas and every role. Keep all valid fonts unchanged.
3. On failure, reset every role and FontDefault, clear the failed atlas and
   rebuild with the incorporated default font once. Validate again. On total
   failure return through existing LauncherVideoFailed before NewFrame.
4. Check desktop and Android compilation/startup-fault regressions. Review
   sibling source consumers separately; do not claim their failures caused374.
5. Integrate under the CAS lock and rebuild a new Android test identity with
   recorded current sources; keep earlier closed APK bytes unchanged.

No arbitrary signal recovery. Local CPU/VM success is separate from Moto and
S22 confirmation. A font fallback can lose custom lettering/Japanese coverage;
report why it activated without claiming full font quality parity.
