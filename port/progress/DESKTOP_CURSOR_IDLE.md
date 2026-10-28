# Desktop cursor inactivity

The game window hides its pointer after four seconds without mouse activity,
on the next event pump. Movement, a button or wheel event restores it. Leaving
the window, losing keyboard focus, hiding or minimizing restores the pointer;
returning starts a fresh four-second interval. Events from other windows do not
reset the game's timer. A failed SDL visibility request is retried.

`port/include/port/cursor_idle.h` owns the shared policy. Native Windows/Linux
use it in `PumpEvents`; their 64-bit hosts use it in `Pump`. The relocatable
guest does not independently change the desktop cursor. Polling happens after
draining events, so movement at the deadline wins without a hide/show flicker.
Controller and keyboard handling are unchanged.

Previously the native desktop path hid after three seconds even without
checking which window had mouse/keyboard focus. The 64-bit hosts had no idle
policy. The replacement covers both paths with one state machine.

Verification: `python port/tools/test_cursor_idle.py` compiles the production
header with real SDL event types and injected SDL clock/focus/visibility calls.
It covers the 3,999/4,000 ms boundary, deadline movement, unrelated windows,
zero-motion events, focus loss, leave/reentry, minimize/restore, hide/show and
failed visibility requests. Windows native32/host64 and Linux native32/host64
also compile/link against their existing SDL libraries. These checks do not
claim physical Wayland/Steam Deck compositor verification.
