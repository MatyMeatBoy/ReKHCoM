#!/bin/bash
# Manual Ports layout: this launcher is next to the rekhcomr36s/ directory.
ports=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P) || exit 1
GAMEDIR="$ports/rekhcomr36s"
cd "$GAMEDIR" || exit 1

for controlfolder in /opt/system/Tools/PortMaster /opt/tools/PortMaster \
    "${XDG_DATA_HOME:-$HOME/.local/share}/PortMaster" "$ports/PortMaster" \
    /roms/ports/PortMaster /roms2/ports/PortMaster; do
    [ -r "$controlfolder/control.txt" ] || continue
    . "$controlfolder/control.txt"
    if [ -n "${CFW_NAME:-}" ] && [ -r "$controlfolder/mod_${CFW_NAME}.txt" ]; then
        . "$controlfolder/mod_${CFW_NAME}.txt"
    fi
    if command -v get_controls >/dev/null 2>&1; then
        get_controls
        if [ -z "${SDL_GAMECONTROLLERCONFIG:-}" ] && [ -n "${sdl_controllerconfig:-}" ]; then
            export SDL_GAMECONTROLLERCONFIG="$sdl_controllerconfig"
        fi
    fi
    break
done

export SDL_VIDEO_DRIVER="${SDL_VIDEO_DRIVER:-kmsdrm}"
export SDL_AUDIO_DRIVER="${SDL_AUDIO_DRIVER:-alsa}"
export REKHCOM_FULLSCREEN=1

if command -v pm_platform_helper >/dev/null 2>&1; then
    pm_platform_helper "$GAMEDIR/binarm64/rekhcom-arm64"
fi
/bin/sh "$GAMEDIR/ReKHCoM.sh" --force64 "$@"
result=$?
if command -v pm_finish >/dev/null 2>&1; then pm_finish; fi
exit "$result"
