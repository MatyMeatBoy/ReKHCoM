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

Source fix only until a separately verified package includes it. Existing
v2.2.5 and the Android #480 test stay immutable; issue479 remains open meanwhile.
