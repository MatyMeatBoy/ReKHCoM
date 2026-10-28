# ReKHCoM on Linux

The desktop/Steam Deck archive contains both x86 (32-bit) and x64 runtimes.
The wrapper chooses x64 on x86-64 systems when available, with x86 as fallback;
the game images retain their 32-bit pointer layout. Anbernic uses armhf (32-bit
ARM), not ARM64. Bundled libraries support portable extraction.

You need your own cartridge dump (EU, US or JP, 32 MiB, unpatched). No ROM is
included. Copy it into `roms/` with any file name: the version is read from
the ROM header. Saves (`kh_com.sav`), settings (`kh_com.ini`) and quick states
(`states/`) are kept in the game folder. The archives have no top folder: to
update, extract the new one over the old install; saves, settings and the ROM
stay.

## Linux desktop and Steam Deck (`linux-x86-x64-desktop-steamdeck`)

    mkdir ReKHCoM && cd ReKHCoM
    tar xf /path/to/ReKHCoM-*-linux-x86-x64-desktop-steamdeck.tar.gz
    cp /path/to/your/rom.gba roms/
    ./ReKHCoM.sh

With several ROMs in `roms/`, pick one with `./ReKHCoM.sh --region us` (`eu`,
`us` or `jp`). Settings remain in the in-game Options menu. For native HD import, open
`ReKHCoM Launcher.sh` in Desktop Mode (KDE/kdialog, Zenity fallback or CLI). F11 or Alt+Enter toggles fullscreen.

It runs under X11 or XWayland and plays sound through PulseAudio/PipeWire or
ALSA. The x64 host uses the system's GPU driver; the x86 fallback needs a
compatible 32-bit driver or uses SDL's software renderer. No ARM64 Linux
runtime is included in this archive. An opt-in experimental source builder is
documented in `port/linuxarm64/README.md` in the repository; it still requires
complete link/runtime and device QA and does not change this package.

## Steam Deck setup (same archive)

Use the same desktop/Deck archive. Device detection defaults to fullscreen on
Steam Deck before reading saved settings, so an existing choice remains valid.
No separate Deck archive or forced configuration preset is needed.

1. In Desktop Mode, extract the archive into a folder of its own (for example
   `~/Games/ReKHCoM`) and copy your ROM into its `roms/` folder.
2. Open **Setup Steam Deck.sh** in Dolphin and choose **Execute** if asked.
   It registers the application and shows the remaining Steam steps, using
   KDE's dialog when available. Alternatively open a terminal in that folder:

       sh ./ReKHCoM.sh --install-desktop

   This registers **ReKHCoM** in your application menu. It needs no ROM to
   install, does not start the game and does not edit Steam. The entry uses
   `/bin/sh` and absolute paths to the script, working folder and icon.
3. In the Steam client, choose Games > **Add a Non-Steam Game to My Library**,
   select **ReKHCoM** from the installed applications and add it. Do not use
   the file manager's **Add to Steam** action on the `.sh` file: some Deck
   versions reject shell scripts with "Unsupported file type" (issue #49).
4. In the shortcut's Properties > Compatibility, leave **Force the use of a
   specific Steam Play compatibility tool** unchecked for this native Linux
   package. Return to Gaming Mode and launch the shortcut. The default
   **Gamepad** Steam Input layout works; the touchscreen is not used.

If ReKHCoM does not appear in the installed-applications list, or an older
shortcut already fails, run `sh ./ReKHCoM.sh --steam-help`. In that shortcut's
Properties > Shortcut, use the exact Target, Start In and Launch Options it
prints. The target is `/bin/sh` (you can Browse to that executable), and the
launch options start with the **quoted absolute path** to `ReKHCoM.sh`.
For the example folder above:

    Target:         /bin/sh
    Start In:       "/home/deck/Games/ReKHCoM"
    Launch Options: "/home/deck/Games/ReKHCoM/ReKHCoM.sh"

Keep the quotes, including when the install path contains spaces. Optional
arguments go **after** the quoted script path, for example `--region us`.

The native 64-bit runtime also accepts `--language en|es|fr|de|it` after the
script path (the last value wins). This is a limited game-option contract:
other 32-bit game arguments are not generally supported by the 64-bit host.
ROM/region selection still uses the wrapper's ROM scan and `--region`; a game
`--rom` argument does not override that selection in 64-bit mode. Host data
paths and bootstrap markers cannot be overridden by appended game arguments.

Keep using the wrapper: `bin/khcom_eu` alone needs the bundled loader and
libraries and is not the Steam target. The wrapper enters the game folder
and passes `--data-dir`, so saves and settings stay there even if Steam starts
from another directory. Script executable permissions are not required when
using `/bin/sh`.

After moving the game folder, run `--install-desktop` again and update the
Steam shortcut from `--steam-help`; Steam's existing shortcuts are not changed
automatically. Older packages' root-level `ReKHCoM.desktop` used relative
paths; use the newly installed application entry instead.

### Where are the launcher settings? (issue #330)

Gaming Mode keeps the direct game shortcut. The separate **ReKHCoM Launcher.sh**
is a basic native Desktop helper for Play and HD import; it is not the Windows GUI. Open **Options** with the bound GBA **Start + Select** buttons
together (the two small menu/view buttons in the default Gamepad layout), or
**F1 / Escape** on a keyboard. The Steam button belongs to Steam. Choose a
**Gamepad** Steam Input layout so those buttons reach the game. Display, HD
components and controls are configured in that menu; HD extraction/import is
still performed with the Windows launcher and copied to the Linux folder.

The setup helper does not launch the game or edit Steam's shortcuts. If you
move the folder, register again and repair the existing Steam shortcut with
`--steam-help`. A successful desktop launch or shell test is not confirmation
of Gamescope/hardware compatibility.

Valve documents adding installed applications in Desktop Mode, then finding
them under Non-Steam in Gaming Mode:
https://help.steampowered.com/en/faqs/view/671A-4453-E8D2-323C/

### Native Linux and Windows through Proton

These are different launch paths. This archive's `.sh`, `bin/` and `lib/`
are native Linux. Proton is for the **Windows** package's `ReKHCoM.exe`; do
not force it on `/bin/sh` or on this native wrapper. Adding the Linux app and
using the absolute wrapper path fixes the shortcut setup; it does not prove
that a separate Windows/Proton problem has been fixed.

If a native shortcut still fails, attach `ReKHCoM.log` (wrapper output) and
`kh_com.log` (game output) from the install folder to your report, together
with the release, SteamOS version, Target/Start In/Launch Options and whether
Compatibility is forced. If `ReKHCoM.log` was not created or its date did not
change, the wrapper did not run far enough to open its log: check the target
and script path first. For the Windows package through Proton, include its
`kh_com.log` and the selected Proton version; an earlier report only working
with Proton Experimental remains unverified without those runtime details.

Not reviewed by Valve: there is no Deck Verified status.

### Optional safe startup (one launch)

If the native shortcut still closes before the game appears, this manual test
uses the existing 32-bit runtime, bundled libraries and SDL software renderer.
It can be slower. It is a compatibility attempt, not a confirmed Gaming Mode fix.

1. In the native Steam shortcut's Launch Options, keep the quoted absolute
   script path and insert `--safe-startup` immediately after it, before any
   game arguments. For example: `"/home/deck/Games/ReKHCoM/ReKHCoM.sh" --safe-startup`.
   Keep Target `/bin/sh`, Start In the game folder, and forced Proton unchecked.
2. Launch once in Gaming Mode. The same data folder is used. The wrapper does
   not delete saves, edit game settings or migrate the saved runtime preference;
   the game's ordinary save/settings behavior is unchanged. The log identifies
   `safe startup: native32, bundled libraries, software renderer; session only`.
3. Remove only `--safe-startup` to return to normal startup and the saved
   runtime choice. If this attempt also fails, keep the new `ReKHCoM.log` and
   `kh_com.log`; do not delete or reset the existing saves/configuration.

This option overrides other runtime-mode flags only for that launch. It does
not change the parent environment, Steam's overlay, the shortcut registration
or media quality. It adds no automatic retry after gameplay starts or after
an intentional signal exit. Quoted paths and existing desktop-entry escaping
remain the normal launch path.

## ARM handhelds (`anbernic-beta`)

**Beta, not yet tested on a real handheld.** Aimed at the Anbernic RG34XX
(H700) and similar devices with Knulli, muOS or ROCKNIX, in the PortMaster
layout.

1. Extract the zip into the ports folder (for example `roms/ports/`): it adds
   `ReKHCoM.sh` and the `rekhcom/` folder.
2. Copy your ROM into `rekhcom/roms/`.
3. Start *ReKHCoM* from the Ports menu. SELECT+START closes it when the
   firmware provides PortMaster's gptokeyb.

Requirements: the firmware's 64-bit kernel must run 32-bit ARM programs, and
the screen must be reachable as `/dev/fb0` (the game draws there directly;
it does not use the GPU). The log is written to `rekhcom/log.txt`.

## Building

See `port/progress/LINUX.md` in the source repository.

## Native HD import (issue336)

Open **ReKHCoM Launcher.sh** in Desktop Mode. KDE/kdialog is preferred, then
Zenity; without either dialog the CLI help remains available. Imports open a
visible konsole/xterm/gnome-terminal for progress and Ctrl+C cancellation. If
none is installed, the GUI refuses to start a hidden import and explains the CLI.
No dependency or Wine install is performed. Close the game before importing.

```sh
sh './ReKHCoM Launcher.sh' --import-hd /path/to/prepared/hd
sh './ReKHCoM Launcher.sh' --import-source '/path/to/owned/HD 1.5+2.5 ReMIX'
sh './ReKHCoM Launcher.sh' --import-sprites '/path/to/owned/HD 1.5+2.5 ReMIX'
```

Python3.9+ is needed. Media extraction uses the bundled Python references;
posters use ffmpeg if already present (optional). Sprite extraction additionally
needs numpy/Pillow already installed; missing dependencies produce a clear error,
never automatic installation. Source extraction is legacy language-unknown,
without video recompression or Windows-GUI/settings parity. Prepared EN/JP
imports retain their existing user-declared provenance; no voice is certified.

Import stages a merged HD copy beside the installation, validates it, preserves
the previous complete hd as hd.previous-<id>, then publishes. Failed/canceled
preparation leaves old hd intact. A cut between directory renames may require
manually restoring that retained backup; no crash-proof filesystem guarantee.
Existing ROM/SRAM/config are never imported or removed. Symlink/overlapping
sources and unfinished journals are rejected. Enough space for staging and
retained previous HD is required. Keep backups until you verify playback.

This supplies a native route for336; it does not establish or fix the unexplained
Windows/Wine crash. A Wine/Proton log/version is still needed for that separate
failure. No separate full graphical launcher or automatic Steam downloads.
