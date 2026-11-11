# Generic controller handoff — 2026-11-11

Branch: `fix/raw-usb-joystick-controls`.

User-authorized scope: one ReKHCoM PR for generic USB controllers, SDL2-format
mapping compatibility, and usable settings access; external N64 ports receive
issues rather than implementation PRs. Keep game input available when Select
alone is pressed; Select + Start and F1 reach settings.

The earlier branch added raw buttons/axes but duplicated the existing
`ControlBindingName` definition and used joystick APIs absent from the guest
bridge. This revision fixes both integration problems, adds hats, persistent
raw handles and hotplug cleanup, event-ordered raw menu input, and filters out
the duplicate raw events emitted by normalized gamepads during capture.

The shared mapping loader runs in real SDL, including Windows/Linux x64 hosts.
An explicitly selected environment mapping file is applied after local files.
The raw joystick fallback is limited to direct SDL builds; bridge builds need
a normalized mapping. Setup and limitations are documented in
`docs/controllers.md` (explicitly tracked despite the repository's docs ignore).

Verification run in an isolated clone and build folders, without player saves:

- `python port/tools/test_controller_bindings.py`: PASS. Real SDL virtual
  gamepad/joystick tests cover buttons, signed axes, hats/diagonals, raw capture,
  chord, menu events, queued taps, ordered presses, hotplug, reference cleanup,
  mapping import and environment-file precedence. Hat input and queued taps
  were each demonstrated failing before their fixes.
- `python port/configure_pc.py --version eu`, followed by
  `ninja -f build/pc/build.ninja -j 4`: PASS, full game and launcher build.
- `python port/tools/test_ui_input.py --build build/pc --cycles 10`: PASS,
  13 cases, including 10 overlay cycles each in field and event contexts.
- `python port/windows64/build_windows64.py --host-only --test`: PASS,
  host ABI, fixed mapping, boot-status and guest BIOS regression checks.
- `python port/windows64/build_windows64.py --regions eu --jobs 4 --test`:
  PASS, full EU bridge guest and Windows x64 host build plus those checks.
- Independent code review: event ordering and mapping-file precedence findings
  fixed and covered; no further actionable findings on re-review.

Physical N64 clone testing and Linux compilation have not been performed.
Do not claim that software can repair hardware reporting only digital values.
No release, player installation change or merge into main is authorized here.

External reports sent using the local MatyMeatBoy GitHub CLI session:

- Birdman64 menu: https://github.com/NoahVl/Birdman64/issues/29
- Birdman64 generic-device setup: https://github.com/NoahVl/Birdman64/issues/30
- RaceWave46 menu: https://github.com/DomazinUS/RaceWave46/issues/16
- RaceWave46 mapping follow-up:
  https://github.com/DomazinUS/RaceWave46/issues/3#issuecomment-6804473818
- RecompFrontend shared menu behavior:
  https://github.com/N64Recomp/RecompFrontend/issues/56
- RecompFrontend generic-device follow-up:
  https://github.com/N64Recomp/RecompFrontend/issues/41#issuecomment-6804474634

These are targeted reports and a shared-component request, not confirmation
that every N64 port has been audited or changed.
