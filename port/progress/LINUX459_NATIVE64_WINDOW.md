# Native Linux64 window creation, issue459

The reporter's startup.2 logs confirm SDL_Init succeeds with X11/PipeWire,
but hidden SDL_CreateWindow never returns. The30-second pre-guest deadline
exits75 and native32/software reaches the title with HD text. Recovery is
confirmed; the64-bit window/GPU path is not.

The Windows cross sysroot materializes symlinks as files. SDL3.4.18's
FindLibraryAndSONAME derives runtime names from REALPATH, so its generated
X11 configuration incorrectly uses development names such as libX11.so.
The package supplies these aliases, potentially loading a second Xlib beside
the system/Steam copy using libX11.so.6. This is a concrete build discrepancy,
not yet the proven explanation of this reporter's hang.

Plan:
1. Derive X11 dynamic names from each real ELF DT_SONAME after CMake
   configuration; retain upstream SDL sources and validate before writing.
   Apply to all Linux cross architectures. Leave audio policy unchanged.
2. Test copied development aliases, valid versioned names, missing libraries
   and invalid metadata without a ROM; prove the existing configuration fails
   the intended runtime-name contract before applying the repair.
3. Rebuild only the affected SDL objects in the existing amd64 cache. Verify
   final binary/runtime selection in an isolated Linux VM, including a
   preloaded Xlib case, normal64 and the established32 fallback.
4. Add bounded diagnostics of actual loaded X11/SDL modules if necessary,
   package a new immutable complete test and send to459. Keep the issue open
   until physical64/GPU startup is confirmed. No shorter deadline or disabled
   GPU presented as a root fix.

Source evidence: upstream SDL release-3.4.18 cmake/sdlchecks.cmake,
FindLibraryAndSONAME; local linux_sysroot.py materialize_links; both original
reporter logs hashed in pruebas/issue459-new-logs/confirmation-20261029.
