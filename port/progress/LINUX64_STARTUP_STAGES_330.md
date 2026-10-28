# Linux64 startup boundaries (#330)

The new reporter log reaches guest image mapping and DIV0 handler installation,
then has no renderer message. That compatibility message is not a divide fault
or proof of guest execution. Existing SDL_Init false and NULL renderer paths
already report errors; four different API nonreturns can produce the same last
line. The ignored 32-bit overlay warning does not identify a fatal cause.

The host now records flushed before/after stages for image scratch release,
SDL info/init, window, renderer, VSync/fullscreen, bridge table, boot marker and
guest entry/return. Each stage records PID and monotonic elapsed microseconds.
SDL failures copy the bounded error before diagnostic calls. Linked/compiled
SDL versions and selected video/audio drivers distinguish the library from an
assumed backend. Environment context is presence flags only, without values or
paths. The wrapper records exit code, marker presence and runtime preference
before its existing fallback decision. A marker is readiness, not proof that
the core completed startup.

Combined SDL flags, call order, initial dimensions/VSync, driver selection and
fallback policy are unchanged. No overlay/environment cleanup, forced X11,
silent backend downgrade, signal recovery or new crash fallback is introduced.
The separate output-controls work can apply negotiated guest settings later;
the initial VSync log alone does not establish the final guest-requested value.

`test_linux64_startup_stages.py --work DIR` runs the actual extracted helper and
startup block: nine negative cases and one positive. False/NULL returns retain
errors and avoid downstream calls; controlled nonreturns leave their distinct
begin stage visible even with buffered stderr followed by `_Exit` (no normal
flush). These controlled stubs demonstrate observability, not a Deck repro.
The actual Linux64 host compiles/links using the cached sysroot. Coordinated
offline Debian snapshot smoke passes with the same host bytes and the real EU
guest07d5 from immutable TAR439441: linked SDL3004018, dummy video/audio,
software renderer, successful boot marker, directed title at frame31 and wrapper
exit0/marker1 from a foreign working directory. A separate real invalid-driver
control logs failed SDL_Init, host4/marker0 and the existing32-bit fallback;
the final nonzero exit is expected because that same invalid driver also rejects32.
`SMOKE-VERIFIED.json` records28 raw postconditions, source/component hashes and
QEMU0 after teardown. This is not Steam/Gamescope/physical Deck reproduction.

Normal guest shutdown may call `H_exit` and terminate the process directly;
`boot` need not return. Therefore missing `guest-core status=returned` is not a
hang when the wrapper records exit0/marker1 and the actual core transition is
present. The first private observer wrongly required that return and exact
frame30 instead of observed31; its FAIL/raw logs remain intact. The positive
was reconciled without rerunning; only the unexecuted negative control ran next.
The core's displayed SDL3.4.0 label belongs to its shim; the host's actual linked
library is SDL3.4.18. Those labels do not establish a library-conflict cause.
Private receipts: `pruebas/issue330-entry-diagnostic/20261028-new-log/`.

Android #374 has a similar early-startup symptom in a different executable:
its C++ launcher precedes ROM/core loading. Source fix444 checks SDL/window/GL/
ImGui failures; the public test412/options retained v2.2 native libraries and
does not contain it. A new Android diagnostic can reuse stage names and record
native entry/return/error beside PID/session and lifecycle. SDLActivity3.4.18
discards nativeRunMain's integer return before finishing the Activity; an
ApplicationExitInfo record alone can therefore miss a native return. This is
not evidence that #330 and #374 share a cause, and no Android behavior changes
or physical-device fix are claimed by this PR.
