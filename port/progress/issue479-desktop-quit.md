# Desktop Quit option (#479)

The Quit row was enabled only for native32 desktop builds. Both Windows64 and
Linux64 deliberately use the shared `PORT_LINUX64_GUEST` image; their shim lacks
`SDL_PushEvent`, so the old row was hidden independently of the HD setting.

Enable the row on desktop64 and defer the existing orderly shutdown through a
common request consumed by the main event pump. Cleanup and exit81 are retained:
the Windows launcher already treats81 as close-all, and the native Linux launcher
execs the game wrapper. Console/Android menu scope is unchanged. The fullscreen
wizard step keeps its original native-window guard instead of inheriting the
expanded Quit capability.

Real-menu controls on Windows native32 and the desktop64 guest cover original
and HD text, cancellation (exit0 only at the later harness bound) and confirmed
quit (exit81). Every case checks the visible row through the opt-in menu audit.
Before the fix, native32 passed and desktop64 hid the row and failed the exit81
contract. ABI/boot checks also pass. This verifies the game-side contract; the
launcher branch is reviewed source, not an end-to-end launcher test, and physical
Linux/Gaming Mode is not certified by these Windows runs.

```sh
python port/tools/test_quit_menu.py --exe build/pc-us/khcom.exe --rom /private/us.gba --ui-source port/src/port_ui.c --out build/quit-native
python port/tools/test_quit_menu.py --exe build/windows64/out/rekhcom64.exe --guest build/windows64/out/guest-us.bin --rom /private/us.gba --ui-source port/ps5/build/guest-us/patched/port_ui.c --out build/quit-guest
```

The observer must read the actual compiled UI source: desktop guest generation
inserts an MMPX enum entry. A first private attempt used the unpatched enum and
lacked the menu-close bound; it was stopped and excluded, not called a game crash.
The corrected observer uses a new private data directory and a bounded overlay.

## Desktop test delivered

[v2.2.5-test479-desktop.1](https://github.com/MatyMeatBoy/ReKHCoM/releases/tag/v2.2.5-test479-desktop.1)
contains all three regions and both 32/64 runtimes for Windows/Linux. Source
`e51c2511366e3dc12e3dcf694342da416619cebb` includes the fix integrated as
`0cd1473e4c0a2b443b20f2685dde052e5c8d1a83`. Windows US32 reuses the build from
`69b5cb14789acfe8eeeb74ba8a2430fee7675552`, with relevant source/header/assets
inputs verified equivalent to the packaging source.

The exact Windows ZIP passed 24 menu cases across EU/US/JP and 32/64. Actual UI
launcher → game → quit flows in both architectures also passed; their frontend
and game bytes match the closed ZIP members. The exact Linux TAR passed 22 menu
and Python-launcher exec cases in an isolated 512 MiB, one-CPU VM. Japanese HD
lettering is not asserted by a requested configuration flag. Dummy audio does
not verify audible continuity, and physical Gaming Mode remains unverified.

| Archive | SHA256 |
| --- | --- |
| Windows ZIP | `e5c088a470a8a821d9fb3d0ce17f03bdc36745ee2cd3c5a301d8590f49de9ab7` |
| Linux TAR | `90828982bbdc2ad095f96f96a7ab2c33835a06a65c31ff266e6325bcf6dd2339` |

All six public assets were anonymously downloaded and matched the closed hashes.
The test was sent to #479, and that reproducible missing-option bug was closed.
#330/#459 received the relevant Linux test while retaining their separate open
startup investigations. Stable v2.2.5 remains Latest and unchanged, as does the
separate Android #480 APK. This test includes the current spring initialization
and optional Linux safe-startup library selection fixes, not a claim that all
device, import-dependency or audio reports are resolved.
