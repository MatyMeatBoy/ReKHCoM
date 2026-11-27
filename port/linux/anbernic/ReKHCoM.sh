#!/bin/bash
# ReKHCoM for ARM handhelds (Anbernic RG34XX and similar), PortMaster layout:
# this script goes in the ports folder next to the rekhcom/ folder. Copy
# your cartridge dump (EU, US or JP) into rekhcom/roms/.
#
# The game is a 32-bit ARM (armhf) program with its own loader and
# libraries; the system's 64-bit kernel must run 32-bit programs. It draws
# straight to the framebuffer (/dev/fb0). SELECT+START closes it when
# PortMaster's gptokeyb is available.

XDG_DATA_HOME=${XDG_DATA_HOME:-$HOME/.local/share}
if [ -d "/opt/system/Tools/PortMaster/" ]; then
  controlfolder="/opt/system/Tools/PortMaster"
elif [ -d "/opt/tools/PortMaster/" ]; then
  controlfolder="/opt/tools/PortMaster"
elif [ -d "$XDG_DATA_HOME/PortMaster/" ]; then
  controlfolder="$XDG_DATA_HOME/PortMaster"
else
  controlfolder="/roms/ports/PortMaster"
fi
if [ -f "$controlfolder/control.txt" ]; then
  # shellcheck disable=SC1091
  source "$controlfolder/control.txt"
  get_controls
  [ -n "$sdl_controllerconfig" ] && export SDL_GAMECONTROLLERCONFIG="$sdl_controllerconfig"
fi

GAMEDIR="$(cd "$(dirname "$0")" && pwd)/rekhcom"
cd "$GAMEDIR" || exit 1
export KHCOM_FBDEV="${KHCOM_FBDEV:-/dev/fb0}"
# KMS page flips (fbdev_linux.c) are verified on the GA36 panel only: the old /dev/fb0 copy here until tested
export KHCOM_KMS="${KHCOM_KMS:-0}"
export SDL_AUDIO_DRIVER="${SDL_AUDIO_DRIVER:-alsa}"

if [ -n "$GPTOKEYB" ]; then
  $GPTOKEYB "rekhcom" &
fi
"$GAMEDIR/ReKHCoM.sh" "$@" > "$GAMEDIR/log.txt" 2>&1
[ -n "$GPTOKEYB" ] && pkill -f gptokeyb
printf "\033c" > /dev/tty0 2>/dev/null
